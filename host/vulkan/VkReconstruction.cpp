// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "VkReconstruction.h"

#include <string.h>

#include <unordered_map>

#include "FrameBuffer.h"
#include "VkDecoder.h"
#include "VulkanBoxedHandles.h"
#include "aemu/base/containers/EntityManager.h"

namespace gfxstream {
namespace vk {
namespace {

uint32_t GetOpcode(const VkSnapshotApiCallInfo& info) {
    if (info.packet.size() <= 4) return -1;

    return *(reinterpret_cast<const uint32_t*>(info.packet.data()));
}

}  // namespace

#define DEBUG_RECONSTRUCTION 1

#if DEBUG_RECONSTRUCTION

#define DEBUG_RECON(fmt, ...) INFO(fmt, ##__VA_ARGS__);

#else

#define DEBUG_RECON(fmt, ...)

#endif

VkReconstruction::VkReconstruction() = default;

std::vector<VkReconstruction::HandleWithState> typeTagSortedHandles(
    const std::vector<VkReconstruction::HandleWithState>& handles) {
    using EntityManagerTypeForHandles = android::base::EntityManager<32, 16, 16, int>;

    std::vector<VkReconstruction::HandleWithState> res = handles;

    std::sort(res.begin(), res.end(),
              [](const VkReconstruction::HandleWithState& lhs,
                 const VkReconstruction::HandleWithState& rhs) {
                  if (lhs.second != rhs.second) {
                      return lhs.second < rhs.second;
                  }
                  return EntityManagerTypeForHandles::getHandleType(lhs.first) <
                         EntityManagerTypeForHandles::getHandleType(rhs.first);
              });

    return res;
}

void VkReconstruction::clear() {
    mApiCallManager.clear();
    mHandleReconstructions.clear();
}

void VkReconstruction::saveReplayBuffers(android::base::Stream* stream) {
    DEBUG_RECON("start")

#if DEBUG_RECONSTRUCTION
    dump();
#endif

    std::vector<uint64_t> uniqApiRefsByTopoOrder;

    mGraph.getApiByTopoOrder(uniqApiRefsByTopoOrder);

    size_t totalApiTraceSize = 0;

        for (auto apiHandle : uniqApiRefsByTopoOrder) {
            const VkSnapshotApiCallInfo* info = mApiCallManager.get(apiHandle);
            totalApiTraceSize += info->packet.size();
        }

    DEBUG_RECON("total api trace size: %zu", totalApiTraceSize);

    std::vector<uint64_t> createdHandleBuffer;

        for (auto apiHandle : uniqApiRefsByTopoOrder) {
            auto item = mApiCallManager.get(apiHandle);
            for (auto createdHandle : item->createdHandles) {
                DEBUG_RECON("save handle: 0x%lx", createdHandle);
                createdHandleBuffer.push_back(createdHandle);
            }
        }

    std::vector<uint8_t> apiTraceBuffer;
    apiTraceBuffer.resize(totalApiTraceSize);

    uint8_t* apiTracePtr = apiTraceBuffer.data();

        for (auto apiHandle : uniqApiRefsByTopoOrder) {
            auto item = mApiCallManager.get(apiHandle);
            // 4 bytes for opcode, and 4 bytes for saveBufferRaw's size field
            DEBUG_RECON("saving api handle 0x%lx op code %d", apiHandle, GetOpcode(*item));
            memcpy(apiTracePtr, item->packet.data(), item->packet.size());
            apiTracePtr += item->packet.size();
        }

    DEBUG_RECON("created handle buffer size: %zu trace: %zu", createdHandleBuffer.size(),
                apiTraceBuffer.size());

    android::base::saveBuffer(stream, createdHandleBuffer);
    android::base::saveBuffer(stream, apiTraceBuffer);
}

/*static*/
void VkReconstruction::loadReplayBuffers(android::base::Stream* stream,
                                         std::vector<uint64_t>* outHandleBuffer,
                                         std::vector<uint8_t>* outDecoderBuffer) {
    DEBUG_RECON("starting to unpack decoder replay buffer");

    android::base::loadBuffer(stream, outHandleBuffer);
    android::base::loadBuffer(stream, outDecoderBuffer);

    DEBUG_RECON("finished unpacking decoder replay buffer");
}

VkSnapshotApiCallInfo* VkReconstruction::createApiCallInfo() {
    VkSnapshotApiCallHandle handle = mApiCallManager.add(VkSnapshotApiCallInfo(), 1);

    auto* info = mApiCallManager.get(handle);
    info->handle = handle;
    return info;
}

void VkReconstruction::removeHandleFromApiInfo(VkSnapshotApiCallHandle h, uint64_t toRemove) {
    return;
    auto vk_item = mHandleReconstructions.get(toRemove);
    if (!vk_item) return;
    auto apiInfo = mApiCallManager.get(h);
    if (!apiInfo) return;

    auto& handles = apiInfo->createdHandles;
    auto it = std::find(handles.begin(), handles.end(), toRemove);

    if (it != handles.end()) {
        handles.erase(it);
    }
    DEBUG_RECON("removed 1 vk handle  0x%llx from apiInfo  0x%llx, now it has %d left",
                (unsigned long long)toRemove, (unsigned long long)h, (int)handles.size());
}

void VkReconstruction::destroyApiCallInfo(VkSnapshotApiCallHandle h) {
    auto item = mApiCallManager.get(h);

    if (!item) return;

    if (!item->createdHandles.empty()) return;

    item->createdHandles.clear();

    mApiCallManager.remove(h);
    mGraph.removeApiNode(h);
}

void VkReconstruction::destroyApiCallInfoIfUnused(VkSnapshotApiCallInfo* info) {
    if (!info) return;
    auto handle = info->handle;
    auto currentInfo = mApiCallManager.get(handle);
    if (!currentInfo) return;

    if (currentInfo->packet.empty()) {
        mApiCallManager.remove(handle);
        mGraph.removeApiNode(handle);
        return;
    }

    if (!info->extraCreatedHandles.empty()) {
        currentInfo->createdHandles.insert(currentInfo->createdHandles.end(), info->extraCreatedHandles.begin(),
                                    info->extraCreatedHandles.end());
        info->extraCreatedHandles.clear();
    }
}

VkSnapshotApiCallInfo* VkReconstruction::getApiInfo(VkSnapshotApiCallHandle h) {
    return mApiCallManager.get(h);
}

void VkReconstruction::setApiTrace(VkSnapshotApiCallInfo* apiInfo, const uint8_t* packet,
                                   size_t packetLenBytes) {
    auto* info = mApiCallManager.get(apiInfo->handle);
    if(info) {
        info->packet.assign(packet, packet + packetLenBytes);
    }
}

void VkReconstruction::dump() {
    INFO("%s: dep graph dump", __func__);

    mGraph.dump(mApiCallManager);

    INFO("%s: api trace dump", __func__);

    return;
    size_t traceBytesTotal = 0;

    mApiCallManager.forEachLiveEntry_const(
        [&traceBytesTotal](bool live, uint64_t handle, const VkSnapshotApiCallInfo& info) {
            const uint32_t opcode = GetOpcode(info);
            INFO("VkReconstruction::%s: api handle 0x%llx: %s", __func__,
                 (unsigned long long)handle, api_opcode_to_string(opcode));
            traceBytesTotal += info.packet.size();
        });

    mHandleReconstructions.forEachLiveComponent_const(
        [this](bool live, uint64_t componentHandle, uint64_t entityHandle,
               const HandleWithStateReconstruction& reconstruction) {
            INFO("VkReconstruction::%s: %p handle 0x%llx api refs:", __func__, this,
                    (unsigned long long)entityHandle);
            for (const auto& state : reconstruction.states) {
                for (auto apiHandle : state.apiRefs) {
                    auto apiInfo = mApiCallManager.get(apiHandle);
                    const char* apiName =
                        apiInfo ? api_opcode_to_string(GetOpcode(*apiInfo)) : "unalloced";
                    INFO("VkReconstruction::%s:     0x%llx: %s", __func__,
                            (unsigned long long)apiHandle, apiName);
                    for (auto createdHandle : apiInfo->createdHandles) {
                        INFO("VkReconstruction::%s:         created 0x%llx", __func__,
                                (unsigned long long)createdHandle);
                    }
                }
            }
        });

    mHandleModifications.forEachLiveComponent_const([this](bool live, uint64_t componentHandle,
                                                           uint64_t entityHandle,
                                                           const HandleModification& modification) {
        INFO("VkReconstruction::%s: mod: %p handle 0x%llx api refs:", __func__, this,
                (unsigned long long)entityHandle);
        for (auto apiHandle : modification.apiRefs) {
            auto apiInfo = mApiCallManager.get(apiHandle);
            const char* apiName = apiInfo ? api_opcode_to_string(GetOpcode(*apiInfo)) : "unalloced";
            INFO("VkReconstruction::%s: mod:     0x%llx: %s", __func__,
                    (unsigned long long)apiHandle, apiName);
        }
    });

    INFO("%s: total trace bytes: %zu", __func__, traceBytesTotal);
}

void VkReconstruction::addHandles(const uint64_t* toAdd, uint32_t count) {
    if (!toAdd) return;

    mGraph.addHandles(toAdd, count);
    return;
    for (uint32_t i = 0; i < count; ++i) {
        DEBUG_RECON("add 0x%llx", (unsigned long long)toAdd[i]);
        mHandleReconstructions.add(toAdd[i], HandleWithStateReconstruction());
    }
}

void VkReconstruction::removeHandles(const uint64_t* toRemove, uint32_t count, bool recursive) {
    if (!toRemove) return;

    return;
    for (uint32_t i = 0; i < count; ++i) {
        DEBUG_RECON("remove 0x%llx", (unsigned long long)toRemove[i]);
        auto item = mHandleReconstructions.get(toRemove[i]);
        // Delete can happen in arbitrary order.
        // It might delete the parents before children, which will automatically remove
        // the name.
        if (!item) continue;
        // Break circuler references
        if (item->destroying) continue;
        item->destroying = true;
        if (!recursive) {
            bool couldDestroy = true;
            for (const auto& state : item->states) {
                if (!state.childHandles.size()) {
                    continue;
                }
                couldDestroy = false;
                break;
            }
            // TODO(b/330769702): perform delayed destroy when all children are destroyed.
            if (couldDestroy) {
                forEachHandleDeleteApi(toRemove + i, 1);
                mHandleReconstructions.remove(toRemove[i]);
            } else {
                DEBUG_RECON("delay destroy of 0x%lx, TODO: actually destroy it", toRemove[i]);
                item->delayed_destroy = true;
                item->destroying = false;
            }
            continue;
        }
        for (size_t j = 0; j < item->states.size(); j++) {
            for (const auto& parentHandle : item->states[j].parentHandles) {
                auto parentItem = mHandleReconstructions.get(parentHandle.first);
                if (!parentItem) {
                    continue;
                }
                parentItem->states[parentHandle.second].childHandles.erase(
                    {toRemove[i], static_cast<HandleState>(j)});
            }
            item->states[j].parentHandles.clear();
            std::vector<uint64_t> childHandles;
            for (const auto& childHandle : item->states[j].childHandles) {
                if (childHandle.second == CREATED) {
                    childHandles.push_back(childHandle.first);
                }
            }
            item->states[j].childHandles.clear();
            removeHandles(childHandles.data(), childHandles.size());
        }
        forEachHandleDeleteApi(toRemove + i, 1);
        mHandleReconstructions.remove(toRemove[i]);
    }
}

void VkReconstruction::forEachHandleAddApi(const uint64_t* toProcess, uint32_t count,
                                           uint64_t apiHandle, HandleState state) {
    if (!toProcess) return;

    // fixme
    if (state == VkReconstruction::CREATED) {
        mGraph.forEachHandleAddApi(toProcess, count, apiHandle);
    }
    return;
    for (uint32_t i = 0; i < count; ++i) {
        auto item = mHandleReconstructions.get(toProcess[i]);
        if (!item) continue;

        item->states[state].apiRefs.push_back(apiHandle);
        DEBUG_RECON("handle 0x%lx state %d added api 0x%lx", toProcess[i], state, apiHandle);
    }
}

void VkReconstruction::forEachHandleDeleteApi(const uint64_t* toProcess, uint32_t count) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {
        DEBUG_RECON("deleting api for 0x%lx", toProcess[i]);
        auto item = mHandleReconstructions.get(toProcess[i]);

        if (!item) continue;

        for (auto& state : item->states) {
            for (auto handle : state.apiRefs) {
                removeHandleFromApiInfo(handle, toProcess[i]);
                destroyApiCallInfo(handle);
            }
            state.apiRefs.clear();
        }

        auto modifyItem = mHandleModifications.get(toProcess[i]);

        if (!modifyItem) continue;

        modifyItem->apiRefs.clear();
    }
}

void VkReconstruction::addHandleDependency(const uint64_t* handles, uint32_t count,
                                           uint64_t parentHandle, HandleState childState,
                                           HandleState parentState) {
    if (!handles) return;

    if (!parentHandle) return;

    mGraph.addHandleDependency(handles, count, parentHandle);

    return;

    auto parentItem = mHandleReconstructions.get(parentHandle);

    if (!parentItem) {
        DEBUG_RECON("WARN: adding null parent item: 0x%lx", parentHandle);
        return;
    }
    auto& parentItemState = parentItem->states[parentState];

    for (uint32_t i = 0; i < count; ++i) {
        auto childItem = mHandleReconstructions.get(handles[i]);
        if (!childItem) {
            continue;
        }
        parentItemState.childHandles.insert({handles[i], static_cast<HandleState>(childState)});
        childItem->states[childState].parentHandles.push_back(
            {parentHandle, static_cast<HandleState>(parentState)});
        DEBUG_RECON("Child handle 0x%lx state %d depends on parent handle 0x%lx state %d",
                    handles[i], childState, parentHandle, parentState);
    }
}

void VkReconstruction::setCreatedHandlesForApi(uint64_t apiHandle, const uint64_t* created,
                                               uint32_t count) {
    if (!created) return;

    mGraph.setCreatedHandlesForApi(apiHandle, created, count);
    auto item = mApiCallManager.get(apiHandle);

    if (!item) return;

    item->createdHandles.insert(item->createdHandles.end(), created, created + count);
}

void VkReconstruction::forEachHandleAddModifyApi(const uint64_t* toProcess, uint32_t count,
                                                 uint64_t apiHandle) {
    if (!toProcess) return;

    return;
    for (uint32_t i = 0; i < count; ++i) {
        auto item = mHandleModifications.get(toProcess[i]);
        if (!item) {
            mHandleModifications.add(toProcess[i], HandleModification());
            item = mHandleModifications.get(toProcess[i]);
        }

        if (!item) continue;

        item->apiRefs.push_back(apiHandle);
    }
}

void VkReconstruction::forEachHandleClearModifyApi(const uint64_t* toProcess, uint32_t count) {
    if (!toProcess) return;

    return;
    for (uint32_t i = 0; i < count; ++i) {
        auto item = mHandleModifications.get(toProcess[i]);

        if (!item) continue;

        item->apiRefs.clear();
    }
}

std::vector<uint64_t> VkReconstruction::getOrderedUniqueModifyApis() const {
    std::vector<HandleModification> orderedModifies;

    // Now add all handle modifications to the trace, ordered by the .order field.
    mHandleModifications.forEachLiveComponent_const(
        [&orderedModifies](bool live, uint64_t componentHandle, uint64_t entityHandle,
                           const HandleModification& mod) { orderedModifies.push_back(mod); });

    // Sort by the |order| field for each modify API
    // since it may be important to apply modifies in a particular
    // order (e.g., when dealing with descriptor set updates
    // or commands in a command buffer).
    std::sort(orderedModifies.begin(), orderedModifies.end(),
              [](const HandleModification& lhs, const HandleModification& rhs) {
                  return lhs.order < rhs.order;
              });

    std::unordered_set<uint64_t> usedModifyApis;
    std::vector<uint64_t> orderedUniqueModifyApis;

    for (const auto& mod : orderedModifies) {
        for (auto apiRef : mod.apiRefs) {
            if (usedModifyApis.find(apiRef) == usedModifyApis.end()) {
                orderedUniqueModifyApis.push_back(apiRef);
                usedModifyApis.insert(apiRef);
            }
        }
    }

    return orderedUniqueModifyApis;
}

// ===================  new graph dependency
void DepGraph::addHandles(const uint64_t* toAdd, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        addDepNode(toAdd[i]);
    }
}

void DepGraph::addDepNode(uint64_t id) {
    if (getDepNode(id)) {
        // this can happen, e.g.
        // vkGetDeviceQueue, can be called
        // multiple times with same queue
        // or enumerate physical device
        // multiple times
        return;
    }
    auto* nd = new DepNode();
    nd->id = id;
    mDepId2DepNode[id] = nd;
    INFO("%s %d id 0x%llx, nd %p", __func__, __LINE__, (unsigned long long)id, nd);
}
void DepGraph::dump(SimpleManager& apiManager) {
    for (const auto& [handle, item] : mDepId2DepNode) {
        auto apiHandle = item->apiRef;
        auto* apiInfo = apiManager.get(apiHandle);
        INFO("handle 0x%llx api 0x%llx name: %s", (unsigned long long)handle, item->apiRef,
             api_opcode_to_string(GetOpcode(*apiInfo)));
    }
}

uint64_t DepGraph::getHandlePuid(uint64_t handle) const { return (handle & 0xFFFF00) >> 8; }

uint64_t DepGraph::getHandleType(uint64_t handle) const { return handle & 0xFF; }

void DepGraph::addHandleDependency(const uint64_t* handles, uint32_t count, uint64_t parentHandle) {
    for (uint32_t i = 0; i < count; ++i) {
        addDep(handles[i], parentHandle);
    }
}

    

void DepGraph::getApiByTopoOrder(std::vector<uint64_t>& uniqApiRefsByTopoOrder) {
    for (const auto& [handle, item] : mDepId2DepNode) {
        auto apiHandle = item->apiRef;
        uniqApiRefsByTopoOrder.push_back(apiHandle);
    }
}

void DepGraph::addDep(uint64_t child_id, uint64_t parent_id) {
    INFO("%s %d child 0x%llx parent 0x%llx", __func__, __LINE__, (unsigned long long)child_id,
         (unsigned long long)parent_id);

    auto ptype = getHandleType(parent_id);
    switch (ptype) {
        case Tag_VkInstance:
        case Tag_VkPhysicalDevice:
        case Tag_VkDevice:
        case Tag_VkDeviceMemory:
        case Tag_VkCommandBuffer:
            break;
        default:
            INFO("%s %d child 0x%llx parent 0x%llx", __func__, __LINE__,
                 (unsigned long long)child_id, (unsigned long long)parent_id);
            return;
    }

    auto* child = getDepNode(child_id);
    auto* parent = getDepNode(parent_id);
    if (!child || !parent) return;

    auto ctype = getHandleType(child_id);
    if (ptype == Tag_VkCommandBuffer) {
        if (ctype == Tag_VkResetCmd) {
            parent->childHandles.clear();
        } else if (ctype == Tag_VkCmdOp) {
            child->parentHandle = parent_id;
            parent->childHandles.insert(child_id);
        }
        INFO("%s %d child 0x%llx parent 0x%llx", __func__, __LINE__, (unsigned long long)child_id,
             (unsigned long long)parent_id);
        return;
    }

    if (ptype == Tag_VkDeviceMemory) {
        if (ctype == Tag_VkBindMemory) {
        } else if(ctype == Tag_VkMapMemory) {
        } else {
            INFO("%s %d child 0x%llx parent 0x%llx", __func__, __LINE__,
                 (unsigned long long)child_id, (unsigned long long)parent_id);
            return;
        }
    }

    child->parentHandle = parent_id;
    parent->childHandles.insert(child_id);
    INFO("%s %d child 0x%llx parent 0x%llx", __func__, __LINE__, (unsigned long long)child_id,
         (unsigned long long)parent_id);
}

// ===================  done graph dependency

}  // namespace vk
}  // namespace gfxstream
