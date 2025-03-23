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
#pragma once

#include <map>
#include <set>

#include "VkSnapshotApiCall.h"
#include "VulkanHandleMapping.h"
#include "VulkanHandles.h"
#include "aemu/base/HealthMonitor.h"
#include "aemu/base/files/Stream.h"
#include "common/goldfish_vk_marshaling.h"
#include "utils/GfxApiLogger.h"

namespace gfxstream {
namespace vk {

// A class that captures all important data structures for
// reconstructing a Vulkan system state via trimmed API record and replay.
class SimpleManager {
   public:
    VkSnapshotApiCallInfo* get(uint64_t handle) {
        auto iter = mApiHandle2Info.find(handle);
        if (iter != mApiHandle2Info.end()) {
            return iter->second;
        }
        return nullptr;
    }

    using EntityHandle = uint64_t;
    using ConstIteratorFunc =
        std::function<void(bool live, EntityHandle h, const VkSnapshotApiCallInfo& item)>;

    void forEachLiveEntry_const(ConstIteratorFunc func) const {
        bool live = true;
        for (auto& [key, val] : mApiHandle2Info) {
            func(live, key, *val);
        }
    }

    uint64_t add(VkSnapshotApiCallInfo info, uint64_t tag) {
        auto* copy = new VkSnapshotApiCallInfo(info);
        auto id = mId;
        mId++;
        mApiHandle2Info[id] = copy;
        return id;
    }

    void clear() {
        for (auto& [key, pval] : mApiHandle2Info) {
            delete pval;
        }
        mApiHandle2Info.clear();
    }
    void remove(uint64_t handle) {
        auto iter = mApiHandle2Info.find(handle);
        if (iter != mApiHandle2Info.end()) {
            delete iter->second;
            mApiHandle2Info.erase(iter);
        }
    }

   private:
    uint64_t mId{1};
    std::map<uint64_t, VkSnapshotApiCallInfo*> mApiHandle2Info;
};

class VkReconstruction {
   public:
    VkReconstruction();

    void clear();

    void saveReplayBuffers(android::base::Stream* stream);
    static void loadReplayBuffers(android::base::Stream* stream,
                                  std::vector<uint64_t>* outHandleBuffer,
                                  std::vector<uint8_t>* outDecoderBuffer);

    enum HandleState { BEGIN = 0, CREATED = 0, BOUND_MEMORY = 1, HANDLE_STATE_COUNT };

    typedef std::pair<uint64_t, HandleState> HandleWithState;
    struct HandleWithStateHash {
        inline size_t operator()(const HandleWithState& v) const {
            std::hash<uint64_t> int_hasher;
            return int_hasher(v.first) ^ int_hasher(v.second);
        }
    };

    struct HandleReconstruction {
        std::vector<VkSnapshotApiCallHandle> apiRefs;
        std::unordered_set<HandleWithState, HandleWithStateHash> childHandles;
        std::vector<HandleWithState> parentHandles;
    };

    struct HandleWithStateReconstruction {
        std::vector<HandleReconstruction> states =
            std::vector<HandleReconstruction>(HANDLE_STATE_COUNT);
        bool delayed_destroy = false;
        bool destroying = false;
    };

    using HandleWithStateReconstructions =
        android::base::UnpackedComponentManager<32, 16, 16, HandleWithStateReconstruction>;

    struct HandleModification {
        std::vector<VkSnapshotApiCallHandle> apiRefs;
        uint32_t order = 0;
    };

    using HandleModifications =
        android::base::UnpackedComponentManager<32, 16, 16, HandleModification>;

    VkSnapshotApiCallInfo* createApiCallInfo();
    void destroyApiCallInfo(VkSnapshotApiCallHandle handle);
    void destroyApiCallInfoIfUnused(VkSnapshotApiCallInfo* info);

    void removeHandleFromApiInfo(VkSnapshotApiCallHandle h, uint64_t toRemove);

    VkSnapshotApiCallInfo* getApiInfo(VkSnapshotApiCallHandle h);

    void setApiTrace(VkSnapshotApiCallInfo* apiInfo, const uint8_t* traceBegin, size_t traceBytes);

    void dump();

    void addHandles(const uint64_t* toAdd, uint32_t count);
    void removeHandles(const uint64_t* toRemove, uint32_t count, bool recursive = true);

    void forEachHandleAddApi(const uint64_t* toProcess, uint32_t count,
                             uint64_t VkSnapshotApiCallHandle, HandleState state = CREATED);
    void forEachHandleDeleteApi(const uint64_t* toProcess, uint32_t count);

    void addHandleDependency(const uint64_t* handles, uint32_t count, uint64_t parentHandle,
                             HandleState childState = CREATED, HandleState parentState = CREATED);

    void setCreatedHandlesForApi(VkSnapshotApiCallHandle handle , const uint64_t* created,
                                 uint32_t count);

    void forEachHandleAddModifyApi(const uint64_t* toProcess, uint32_t count,
                                   VkSnapshotApiCallHandle handle);

    void forEachHandleClearModifyApi(const uint64_t* toProcess, uint32_t count);

    void setModifiedHandlesForApi(VkSnapshotApiCallHandle handle, const uint64_t* modified,
                                  uint32_t count);

    // Used by on_vkCreateDescriptorPool.
    //
    // Snapshot keeps track of all the boxed handles created by each function. By default
    // the generated code assumes no extra internal boxed handles are generated by
    // VkDecoderGlobalState. But this is not the case for on_vkCreateDescriptorPool.
    // Thus we add an extra API to VkReconstruction, which gives it the list of all the
    // extra boxed handles.
    //
    // Implementation-wise it is a bit tricky. The regular workflow looks like:
    //
    // on_vkCreateDescriptorPool(... pDescriptorPool)
    // ...
    // mReconstruction.setCreatedHandlesForApi(OP_vkCreateDescriptorPool, pDescriptorPool);
    //
    // It is not easy to directly tell mReconstruction that OP_vkCreateDescriptorPool created
    // extra handles. Instead, we add an API to VkReconstruction to cache the extra handles.
    // Next time setCreatedHandlesForApi is called, it will check the cached handles and
    // add them to OP_vkCreateDescriptorPool.
    void createExtraHandlesForNextApi(const uint64_t* created, uint32_t count);

   private:
    std::vector<uint64_t> getOrderedUniqueModifyApis() const;

    //    VkSnapshotApiCallManager mApiCallManager;
    SimpleManager mApiCallManager;

    HandleWithStateReconstructions mHandleReconstructions;
    HandleModifications mHandleModifications;

    std::vector<uint8_t> mLoadedTrace;
};

struct InternalHandle {
    uint64_t id;
    uint64_t vkHandle;
};

class VkHandleMapper {
   public:
    uint64_t vkHandle2Id(uint64_t vkHandle) {
        if (mVkHandle2InternalHandle.find(vkHandle) == mVkHandle2InternalHandle.end()) {
            mVkHandle2InternalHandle[vkHandle].id = mId;
            mVkHandle2InternalHandle[vkHandle].vkHandle = vkHandle;
            ++mId;
        }
        return mVkHandle2InternalHandle[vkHandle].id;
    }

   private:
    uint64_t mId{1};
    std::map<uint64_t, InternalHandle> mVkHandle2InternalHandle;
};

/*

   // those tag ids are important, as they mark the different
   // types of vk object
enum BoxedHandleTypeTag {
  Tag_Invalid = 0,
  Tag_VkInstance = 1,
  Tag_VkPhysicalDevice = 2,
  Tag_VkDevice = 3,
  Tag_VkQueue = 4,
  Tag_VkDeviceMemory = 5,
  Tag_VkBuffer = 6,
  Tag_VkImage = 7,
  Tag_VkBufferView,
  Tag_VkImageView,
  Tag_VkShaderModule,
  Tag_VkDescriptorSetLayout,
  Tag_VkDescriptorPool,
  Tag_VkDescriptorSet,
  Tag_VkSampler,
  Tag_VkSamplerYcbcrConversion,
  Tag_VkDescriptorUpdateTemplate,
  Tag_VkRenderPass,
  Tag_VkFramebuffer,
  Tag_VkPipelineLayout,
  Tag_VkPipelineCache,
  Tag_VkPipeline,
  Tag_VkFence,
  Tag_VkSemaphore,
  Tag_VkEvent,
  Tag_VkQueryPool,
  Tag_VkSurfaceKHR,
  Tag_VkSwapchainKHR,
  Tag_VkDisplayKHR,
  Tag_VkDisplayModeKHR,
  Tag_VkValidationCacheEXT,
  Tag_VkDebugReportCallbackEXT,
  Tag_VkDebugUtilsMessengerEXT,
  Tag_VkCommandPool,
  Tag_VkCommandBuffer = 34, // 0x22
  Tag_VkAccelerationStructureNV,
  Tag_VkIndirectCommandsLayoutNV,
  Tag_VkAccelerationStructureKHR,
  Tag_VkCuModuleNVX,
  Tag_VkCuFunctionNVX,
  Tag_VkPrivateDataSlot,
  Tag_VkMicromapEXT,

  Tag_VkGeneric = 1001,
};


*/
struct DepNode {
    // id of this depnode, 0 is invalid
    uint64_t id{0};
    // the api that created this DepNode; 0 is invalid
    uint64_t apiRef{0};
    std::set<uint64_t> childHandles;
    // there could be only one parent, 0 is invalid
    uint64_t parentHandle{0};
};

struct ApiNode {
    // id of this api node, 0 is invalid
    uint64_t id{0};
    std::set<uint64_t> createdHandles;
};

class DepGraph {
   public:
    void addHandles(const uint64_t* toAdd, uint32_t count) {
        for (uint32_t i = 0; i < count; ++i) {
            addDepNode(toAdd[i]);
        }
    }
    void addHandleDependency(const uint64_t* handles, uint32_t count, uint64_t parentHandle) {
        for (uint32_t i = 0; i < count; ++i) {
            addDep(handles[i], parentHandle);
        }
    }

    void forEachHandleAddApi(const uint64_t* created, uint32_t count, uint64_t apiRef) {
        for (uint64_t i = 0; i < count; ++i) {
            auto* nd = getDepNode(created[i]);
            if (nd) {
                nd->apiRef = apiRef;
            }
        }
    }

    void replaceDep(uint64_t child_id, uint64_t parent_id) {
        clearChildHandles(parent_id);
        addDep(child_id, parent_id);
    }

    void addDep(uint64_t child_id, uint64_t parent_id) {
        auto* child = getDepNode(child_id);
        auto* parent = getDepNode(parent_id);
        if (!child || !parent) return;
        child->parentHandle = parent_id;
        parent->childHandles.insert(child_id);
    }

    DepNode* getDepNode(uint64_t id) {
        if (mDepId2DepNode.find(id) == mDepId2DepNode.end()) return nullptr;
        auto* nd = mDepId2DepNode[id];
        return nd;
    }

    ApiNode* getApiNode(uint64_t id) {
        if (mApiId2ApiNode.find(id) == mApiId2ApiNode.end()) return nullptr;
        auto* nd = mApiId2ApiNode[id];
        return nd;
    }

    void clearChildHandles(uint64_t id) {
        auto* nd = getDepNode(id);
        if (nd) {
            nd->childHandles.clear();
        }
    }

    void removeHandles(const uint64_t* toRemove, uint32_t count) {
        for (uint32_t i = 0; i < count; ++i) {
            removeDepNode(toRemove[i]);
        }
    }
    void setCreatedHandlesForApi(uint64_t apiRef, const uint64_t* created, uint32_t count) {
        auto* apiNode = getApiNode(apiRef);
        if (apiNode) {
            for (uint32_t i = 0; i < count; ++i) {
                apiNode->createdHandles.insert(created[i]);
            }
        }
    }
    void addApiNode(uint64_t id) {
        auto* nd = new ApiNode();
        nd->id = id;
        mApiId2ApiNode[id] = nd;
    }
    void removeApiNode(uint64_t id) {
        auto* nd = getApiNode(id);
        if (nd) {
            delete nd;
        }
        mApiId2ApiNode.erase(id);
    }
    void addDepNode(uint64_t id) {
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
    }
    void removeDepNode(uint64_t id) {
        auto* nd = getDepNode(id);
        if (nd) {
            for (auto child : nd->childHandles) {
                removeDepNode(child);
            }
            delete nd;
        }
        mDepId2DepNode.erase(id);
    }

   private:
    std::map<uint64_t, DepNode*> mDepId2DepNode;
    std::map<uint64_t, ApiNode*> mApiId2ApiNode;
};

}  // namespace vk
}  // namespace gfxstream
