// Copyright (C) 2025 The Android Open Source Project
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

#include "VulkanBoxedHandles.h"

#include "VkDecoderGlobalState.h"
#include "RenderThreadInfoVk.h"
#include "VkDecoderInternalStructs.h"

namespace gfxstream {
namespace vk {
namespace {

struct ReadStreamRegistry {
    android::base::Lock mLock;

    std::vector<VulkanMemReadingStream*> freeStreams;

    ReadStreamRegistry() { freeStreams.reserve(100); };

    VulkanMemReadingStream* pop(const gfxstream::host::FeatureSet& features) {
        android::base::AutoLock lock(mLock);
        if (freeStreams.empty()) {
            return new VulkanMemReadingStream(nullptr, features);
        } else {
            VulkanMemReadingStream* res = freeStreams.back();
            freeStreams.pop_back();
            return res;
        }
    }

    void push(VulkanMemReadingStream* stream) {
        android::base::AutoLock lock(mLock);
        freeStreams.push_back(stream);
    }
};

static ReadStreamRegistry sReadStreamRegistry;

}  // namespace

void BoxedHandleManager::replayHandles(std::vector<BoxedHandle> handles) {
    mHandleReplay = true;
    mHandleReplayQueue.clear();
    for (BoxedHandle handle : handles) {
        mHandleReplayQueue.push_back(handle);
    }
}

void BoxedHandleManager::clear() {
    std::lock_guard<std::mutex> lock(mMutex);
    mReverseMap.clear();
    mStore.clear();
}

BoxedHandle BoxedHandleManager::add(const BoxedHandleInfo& item, BoxedHandleTypeTag tag) {
    BoxedHandle handle;

    if (mHandleReplay) {
        handle = mHandleReplayQueue.front();
        mHandleReplayQueue.pop_front();
        mHandleReplay = !mHandleReplayQueue.empty();

        handle = (BoxedHandle)mStore.addFixed(handle, item, (size_t)tag);
    } else {
        handle = (BoxedHandle)mStore.add(item, (size_t)tag);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mReverseMap[(BoxedHandle)(item.underlying)] = handle;
    return handle;
}

void BoxedHandleManager::update(BoxedHandle handle, const BoxedHandleInfo& item,
                                BoxedHandleTypeTag tag) {
    auto storedItem = mStore.get(handle);
    UnboxedHandle oldHandle = (UnboxedHandle)storedItem->underlying;
    *storedItem = item;
    std::lock_guard<std::mutex> lock(mMutex);
    if (oldHandle) {
        mReverseMap.erase(oldHandle);
    }
    mReverseMap[(UnboxedHandle)(item.underlying)] = handle;
}

void BoxedHandleManager::remove(BoxedHandle h) {
    auto item = get(h);
    if (item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mReverseMap.erase((UnboxedHandle)(item->underlying));
    }
    mStore.remove(h);
}

void BoxedHandleManager::removeDelayed(uint64_t h, VkDevice device,
                                       std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mDelayedRemoves[device].push_back({h, callback});
}

void BoxedHandleManager::processDelayedRemoves(VkDevice device) {
    std::vector<DelayedRemove> deviceDelayedRemoves;

    {
        std::lock_guard<std::mutex> lock(mMutex);

        auto it = mDelayedRemoves.find(device);
        if (it == mDelayedRemoves.end()) return;

        deviceDelayedRemoves = std::move(it->second);
        mDelayedRemoves.erase(it);
    }

    for (const auto& r : deviceDelayedRemoves) {
        auto h = r.handle;

        // VkDecoderGlobalState is not locked when callback is called.
        if (r.callback) {
            r.callback();
        }

        mStore.remove(h);
    }
}

BoxedHandleInfo* BoxedHandleManager::get(BoxedHandle handle) {
    return (BoxedHandleInfo*)mStore.get_const(handle);
}

BoxedHandle BoxedHandleManager::getBoxedFromUnboxed(UnboxedHandle unboxed) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mReverseMap.find(unboxed);
    if (it == mReverseMap.end()) {
        return 0;
    }

    return it->second;
}

BoxedHandleManager sBoxedHandleManager;

template <typename VkObjectT>
constexpr BoxedHandleTypeTag GetTag() {
    if constexpr (std::is_same_v<VkObjectT, VkAccelerationStructureKHR>) {
        return Tag_VkAccelerationStructureKHR;
    } else if constexpr (std::is_same_v<VkObjectT, VkAccelerationStructureNV>) {
        return Tag_VkAccelerationStructureNV;
    } else if constexpr (std::is_same_v<VkObjectT, VkBuffer>) {
        return Tag_VkBuffer;
    } else if constexpr (std::is_same_v<VkObjectT, VkBufferView>) {
        return Tag_VkBufferView;
    } else if constexpr (std::is_same_v<VkObjectT, VkCommandBuffer>) {
        return Tag_VkCommandBuffer;
    } else if constexpr (std::is_same_v<VkObjectT, VkCommandPool>) {
        return Tag_VkCommandPool;
    } else if constexpr (std::is_same_v<VkObjectT, VkCuFunctionNVX>) {
        return Tag_VkCuFunctionNVX;
    } else if constexpr (std::is_same_v<VkObjectT, VkCuModuleNVX>) {
        return Tag_VkCuModuleNVX;
    } else if constexpr (std::is_same_v<VkObjectT, VkDebugReportCallbackEXT>) {
        return Tag_VkDebugReportCallbackEXT;
    } else if constexpr (std::is_same_v<VkObjectT, VkDebugUtilsMessengerEXT>) {
        return Tag_VkDebugUtilsMessengerEXT;
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorPool>) {
        return Tag_VkDescriptorPool;
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorSet>) {
        return Tag_VkDescriptorSet;
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorSetLayout>) {
        return Tag_VkDescriptorSetLayout;
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorUpdateTemplate>) {
        return Tag_VkDescriptorUpdateTemplate;
    } else if constexpr (std::is_same_v<VkObjectT, VkDevice>) {
        return Tag_VkDevice;
    } else if constexpr (std::is_same_v<VkObjectT, VkDeviceMemory>) {
        return Tag_VkDeviceMemory;
    } else if constexpr (std::is_same_v<VkObjectT, VkDisplayKHR>) {
        return Tag_VkDisplayKHR;
    } else if constexpr (std::is_same_v<VkObjectT, VkDisplayModeKHR>) {
        return Tag_VkDisplayModeKHR;
    } else if constexpr (std::is_same_v<VkObjectT, VkEvent>) {
        return Tag_VkEvent;
    } else if constexpr (std::is_same_v<VkObjectT, VkFence>) {
        return Tag_VkFence;
    } else if constexpr (std::is_same_v<VkObjectT, VkFramebuffer>) {
        return Tag_VkFramebuffer;
    } else if constexpr (std::is_same_v<VkObjectT, VkImage>) {
        return Tag_VkImage;
    } else if constexpr (std::is_same_v<VkObjectT, VkImageView>) {
        return Tag_VkImageView;
    } else if constexpr (std::is_same_v<VkObjectT, VkIndirectCommandsLayoutNV>) {
        return Tag_VkIndirectCommandsLayoutNV;
    } else if constexpr (std::is_same_v<VkObjectT, VkInstance>) {
        return Tag_VkInstance;
    } else if constexpr (std::is_same_v<VkObjectT, VkMicromapEXT>) {
        return Tag_VkMicromapEXT;
    } else if constexpr (std::is_same_v<VkObjectT, VkPhysicalDevice>) {
        return Tag_VkPhysicalDevice;
    } else if constexpr (std::is_same_v<VkObjectT, VkPipeline>) {
        return Tag_VkPipeline;
    } else if constexpr (std::is_same_v<VkObjectT, VkPipelineCache>) {
        return Tag_VkPipelineCache;
    } else if constexpr (std::is_same_v<VkObjectT, VkPipelineLayout>) {
        return Tag_VkPipelineLayout;
    } else if constexpr (std::is_same_v<VkObjectT, VkPrivateDataSlot>) {
        return Tag_VkPrivateDataSlot;
    } else if constexpr (std::is_same_v<VkObjectT, VkQueryPool>) {
        return Tag_VkQueryPool;
    } else if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        return Tag_VkQueue;
    } else if constexpr (std::is_same_v<VkObjectT, VkRenderPass>) {
        return Tag_VkRenderPass;
    } else if constexpr (std::is_same_v<VkObjectT, VkSampler>) {
        return Tag_VkSampler;
    } else if constexpr (std::is_same_v<VkObjectT, VkSamplerYcbcrConversion>) {
        return Tag_VkSamplerYcbcrConversion;
    } else if constexpr (std::is_same_v<VkObjectT, VkSemaphore>) {
        return Tag_VkSemaphore;
    } else if constexpr (std::is_same_v<VkObjectT, VkShaderModule>) {
        return Tag_VkShaderModule;
    } else if constexpr (std::is_same_v<VkObjectT, VkSurfaceKHR>) {
        return Tag_VkSurfaceKHR;
    } else if constexpr (std::is_same_v<VkObjectT, VkSwapchainKHR>) {
        return Tag_VkSwapchainKHR;
    } else if constexpr (std::is_same_v<VkObjectT, VkValidationCacheEXT>) {
        return Tag_VkValidationCacheEXT;
    } else {
        static_assert(sizeof(VkObjectT) == 0,
                      "Unhandled VkObjectT. Please update BoxedHandleTypeTag.");
    }
}

template <typename VkObjectT>
constexpr const char* GetTypeStr() {
    if constexpr (std::is_same_v<VkObjectT, VkAccelerationStructureKHR>) {
        return "VkAccelerationStructureKHR";
    } else if constexpr (std::is_same_v<VkObjectT, VkAccelerationStructureNV>) {
        return "VkAccelerationStructureNV";
    } else if constexpr (std::is_same_v<VkObjectT, VkBuffer>) {
        return "VkBuffer";
    } else if constexpr (std::is_same_v<VkObjectT, VkBufferView>) {
        return "VkBufferView";
    } else if constexpr (std::is_same_v<VkObjectT, VkCommandBuffer>) {
        return "VkCommandBuffer";
    } else if constexpr (std::is_same_v<VkObjectT, VkCommandPool>) {
        return "VkCommandPool";
    } else if constexpr (std::is_same_v<VkObjectT, VkCuFunctionNVX>) {
        return "VkCuFunctionNVX";
    } else if constexpr (std::is_same_v<VkObjectT, VkCuModuleNVX>) {
        return "VkCuModuleNVX";
    } else if constexpr (std::is_same_v<VkObjectT, VkDebugReportCallbackEXT>) {
        return "VkDebugReportCallbackEXT";
    } else if constexpr (std::is_same_v<VkObjectT, VkDebugUtilsMessengerEXT>) {
        return "VkDebugUtilsMessengerEXT";
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorPool>) {
        return "VkDescriptorPool";
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorSet>) {
        return "VkDescriptorSet";
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorSetLayout>) {
        return "VkDescriptorSetLayout";
    } else if constexpr (std::is_same_v<VkObjectT, VkDescriptorUpdateTemplate>) {
        return "VkDescriptorUpdateTemplate";
    } else if constexpr (std::is_same_v<VkObjectT, VkDevice>) {
        return "VkDevice";
    } else if constexpr (std::is_same_v<VkObjectT, VkDeviceMemory>) {
        return "VkDeviceMemory";
    } else if constexpr (std::is_same_v<VkObjectT, VkDisplayKHR>) {
        return "VkDisplayKHR";
    } else if constexpr (std::is_same_v<VkObjectT, VkDisplayModeKHR>) {
        return "VkDisplayModeKHR";
    } else if constexpr (std::is_same_v<VkObjectT, VkEvent>) {
        return "VkEvent";
    } else if constexpr (std::is_same_v<VkObjectT, VkFence>) {
        return "VkFence";
    } else if constexpr (std::is_same_v<VkObjectT, VkFramebuffer>) {
        return "VkFramebuffer";
    } else if constexpr (std::is_same_v<VkObjectT, VkImage>) {
        return "VkImage";
    } else if constexpr (std::is_same_v<VkObjectT, VkImageView>) {
        return "VkImageView";
    } else if constexpr (std::is_same_v<VkObjectT, VkIndirectCommandsLayoutNV>) {
        return "VkIndirectCommandsLayoutNV";
    } else if constexpr (std::is_same_v<VkObjectT, VkInstance>) {
        return "VkInstance";
    } else if constexpr (std::is_same_v<VkObjectT, VkMicromapEXT>) {
        return "VkMicromapEXT";
    } else if constexpr (std::is_same_v<VkObjectT, VkPhysicalDevice>) {
        return "VkPhysicalDevice";
    } else if constexpr (std::is_same_v<VkObjectT, VkPipeline>) {
        return "VkPipeline";
    } else if constexpr (std::is_same_v<VkObjectT, VkPipelineCache>) {
        return "VkPipelineCache";
    } else if constexpr (std::is_same_v<VkObjectT, VkPipelineLayout>) {
        return "VkPipelineLayout";
    } else if constexpr (std::is_same_v<VkObjectT, VkPrivateDataSlot>) {
        return "VkPrivateDataSlot";
    } else if constexpr (std::is_same_v<VkObjectT, VkQueryPool>) {
        return "VkQueryPool";
    } else if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        return "VkQueue";
    } else if constexpr (std::is_same_v<VkObjectT, VkRenderPass>) {
        return "VkRenderPass";
    } else if constexpr (std::is_same_v<VkObjectT, VkSampler>) {
        return "VkSampler";
    } else if constexpr (std::is_same_v<VkObjectT, VkSamplerYcbcrConversion>) {
        return "VkSamplerYcbcrConversion";
    } else if constexpr (std::is_same_v<VkObjectT, VkSemaphore>) {
        return "VkSemaphore";
    } else if constexpr (std::is_same_v<VkObjectT, VkShaderModule>) {
        return "VkShaderModule";
    } else if constexpr (std::is_same_v<VkObjectT, VkSurfaceKHR>) {
        return "VkSurfaceKHR";
    } else if constexpr (std::is_same_v<VkObjectT, VkSwapchainKHR>) {
        return "VkSwapchainKHR";
    } else if constexpr (std::is_same_v<VkObjectT, VkValidationCacheEXT>) {
        return "VkValidationCacheEXT";
    } else {
        static_assert(sizeof(VkObjectT) == 0,
                      "Unhandled VkObjectT. Please update BoxedHandleTypeTag.");
    }
}

static std::string getThreadID() {
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    std::string result = ss.str();
    return result;
}

template <typename VkObjectT>
VkObjectT new_boxed_VkType(VkObjectT underlying, bool dispatchable = false, VulkanDispatch* dispatch = nullptr, bool ownsDispatch = false) {
    new_boxed_VkType<VkObjectT>(&sBoxedHandleManager, underlying, dispatchable, dispatch, ownsDispatch);
}

template <typename VkObjectT>
VkObjectT new_boxed_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT underlying, bool dispatchable = false, VulkanDispatch* dispatch = nullptr, bool ownsDispatch = false) {
    uint32_t additionalTag = 0x0;
    {
//   RenderThreadInfo *tInfo = RenderThreadInfo::get();
            auto* renderThreadInfo = RenderThreadInfoVk::get();
            if (renderThreadInfo) {
                additionalTag = renderThreadInfo->m_vkDec.getVkDecoderGlobalState()->getId();
                fprintf(stderr, "tid %s gs id %d\n",
                        getThreadID().c_str(), (int)additionalTag);
            }
            additionalTag = additionalTag << 8; // left shift 8bit
 //   tInfo->m_puid = puid;
    }
    BoxedHandleInfo info;
    info.underlying = (uint64_t)underlying;
    if (dispatchable) {
        if (dispatch != nullptr) {
            info.dispatch = dispatch;
        } else {
            info.dispatch = new VulkanDispatch();
        }
        info.ownDispatch = ownsDispatch;
        info.ordMaintInfo = new OrderMaintenanceInfo();
        info.readStream = nullptr;
    }
    auto boxed = (VkObjectT)pBoxedHandleManager->add(info,
            (BoxedHandleTypeTag)(GetTag<VkObjectT>() + additionalTag));
    fprintf(stderr, "tid %s unboxed 0x%llx to boxed 0x%llx\n",
            getThreadID().c_str(), (unsigned long long)underlying, (unsigned long long)boxed);
    auto vkboxed = (VkObjectT)boxed;
    return vkboxed;
}

template <typename VkObjectT>
void delete_VkType(VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return;
    }

    BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        return;
    }

    releaseOrderMaintInfo(info->ordMaintInfo);

    if (info->readStream) {
        sReadStreamRegistry.push(info->readStream);
        info->readStream = nullptr;
    }

    sBoxedHandleManager.remove((uint64_t)boxed);
}

template <typename VkObjectT>
void delayed_delete_VkType(VkObjectT boxed, VkDevice device, std::function<void()> callback) {
    if (boxed == VK_NULL_HANDLE) {
        return;
    }

    sBoxedHandleManager.removeDelayed((uint64_t)boxed, device, std::move(callback));
}

// Custom unbox_* functions or GOLDFISH_VK_LIST_DISPATCHABLE_CUSTOM_UNBOX_HANDLE_TYPES
// VkQueue objects can be virtual, meaning that multiple boxed queues can map into a single
// physical queue on the host GPU. Some conversion is needed for unboxing to physical.
VkQueue unbox_VkQueueImpl(VkQueue boxed) {
    BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
    if (!info) {
        return VK_NULL_HANDLE;
    }
    const uint64_t unboxedQueue64 = info->underlying;

    // Use VulkanVirtualQueue directly to avoid locking for hasVirtualGraphicsQueue call.
    if (VkDecoderGlobalState::get()->getFeatures().VulkanVirtualQueue.enabled) {
        // Clear virtual bit and unbox into the actual physical queue handle
        return (VkQueue)(unboxedQueue64 & ~QueueInfo::kVirtualQueueBit);
    }

    return (VkQueue)(unboxedQueue64);
}

template <typename VkObjectT>
VkObjectT unbox_VkType(VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkObjectT unboxed = VK_NULL_HANDLE;

    if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        unboxed = unbox_VkQueueImpl(boxed);
    } else {
        BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
        if (info == nullptr) {
            if constexpr (std::is_same_v<VkObjectT, VkCommandBuffer> ||
                          std::is_same_v<VkObjectT, VkDevice> ||
                          std::is_same_v<VkObjectT, VkInstance> ||
                          std::is_same_v<VkObjectT, VkPhysicalDevice> ||
                          std::is_same_v<VkObjectT, VkQueue>) {
                ERR("Failed to unbox %s %p", GetTypeStr<VkObjectT>(), boxed);
            } else if constexpr (std::is_same_v<VkObjectT, VkFence>) {
                // TODO: investigate.
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                        << "Failed to unbox "
                        << GetTypeStr<VkObjectT>()
                        << " "
                        << boxed
                        << ", not found.";
            }
            unboxed = VK_NULL_HANDLE;
        } else {
            unboxed = (VkObjectT)info->underlying;
        }
    }

    return unboxed;
}

template <typename VkObjectT>
VkObjectT try_unbox_VkType(VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkObjectT unboxed = VK_NULL_HANDLE;

    if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        unboxed = unbox_VkQueueImpl(boxed);
    } else {
        BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
        if (info != nullptr) {
            unboxed = (VkObjectT)info->underlying;
        }
    }

    if (unboxed == VK_NULL_HANDLE) {
        WARN("Failed to try unbox %s %p", GetTypeStr<VkObjectT>(), boxed);
    }

    return unboxed;
}

template <typename VkObjectT>
VkObjectT unboxed_to_boxed_non_dispatchable_VkType(VkObjectT unboxed) {
    if (unboxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    return (VkObjectT)sBoxedHandleManager.getBoxedFromUnboxed((uint64_t)(uintptr_t)unboxed);
}

template <typename VkObjectT>
void set_boxed_non_dispatchable_VkType(VkObjectT boxed, VkObjectT new_unboxed) {
    BoxedHandleInfo info;
    info.underlying = (uint64_t)new_unboxed;
    sBoxedHandleManager.update((uint64_t)boxed, info, GetTag<VkObjectT>());
}

template <typename VkObjectT>
OrderMaintenanceInfo* get_order_maintenance_info_VkType(VkObjectT boxed) {
    BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        return nullptr;
    }

    if (info->ordMaintInfo == nullptr) {
        return nullptr;
    }

    acquireOrderMaintInfo(info->ordMaintInfo);

    return info->ordMaintInfo;
}

template <typename VkObjectT>
VulkanMemReadingStream* get_read_stream_VkType(VkObjectT boxed) {
    BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        return nullptr;
    }

    if (info->readStream == nullptr) {
        info->readStream = sReadStreamRegistry.pop(VkDecoderGlobalState::get()->getFeatures());
    }

    return info->readStream;
}

template <typename VkObjectT>
VulkanDispatch* get_dispatch_VkType(VkObjectT boxed) {
    BoxedHandleInfo* info = sBoxedHandleManager.get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        ERR("Failed to unbox %s %p", GetTypeStr<VkObjectT>(), boxed);
        return nullptr;
    }
    return info->dispatch;
}

///////////////////////////////////////////////////////////////////////////////
//////////////             DISPATCHABLE TYPES                    //////////////
///////////////////////////////////////////////////////////////////////////////

VkInstance new_boxed_VkInstance(VkInstance unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
    return new_boxed_VkType<VkInstance>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
}

void delete_VkInstance(VkInstance boxed) {
    delete_VkType(boxed);
}

VkInstance unbox_VkInstance(VkInstance boxed) {
    return unbox_VkType<VkInstance>(boxed);
}

VkInstance try_unbox_VkInstance(VkInstance boxed) {
    return try_unbox_VkType<VkInstance>(boxed);
}

VkInstance unboxed_to_boxed_VkInstance(VkInstance unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkInstance>(unboxed);
}

OrderMaintenanceInfo* ordmaint_VkInstance(VkInstance boxed) {
    return get_order_maintenance_info_VkType<VkInstance>(boxed);
}

VulkanMemReadingStream* readstream_VkInstance(VkInstance boxed) {
    return get_read_stream_VkType<VkInstance>(boxed);
}

VulkanDispatch* dispatch_VkInstance(VkInstance boxed) {
    return get_dispatch_VkType<VkInstance>(boxed);
}

VkPhysicalDevice new_boxed_VkPhysicalDevice(VkPhysicalDevice unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
    return new_boxed_VkType<VkPhysicalDevice>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
}

void delete_VkPhysicalDevice(VkPhysicalDevice boxed) {
    delete_VkType(boxed);
}

VkPhysicalDevice unbox_VkPhysicalDevice(VkPhysicalDevice boxed) {
    return unbox_VkType<VkPhysicalDevice>(boxed);
}

VkPhysicalDevice try_unbox_VkPhysicalDevice(VkPhysicalDevice boxed) {
    return try_unbox_VkType<VkPhysicalDevice>(boxed);
}

VkPhysicalDevice unboxed_to_boxed_VkPhysicalDevice(VkPhysicalDevice unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPhysicalDevice>(unboxed);
}

OrderMaintenanceInfo* ordmaint_VkPhysicalDevice(VkPhysicalDevice boxed) {
    return get_order_maintenance_info_VkType<VkPhysicalDevice>(boxed);
}

VulkanMemReadingStream* readstream_VkPhysicalDevice(VkPhysicalDevice boxed) {
    return get_read_stream_VkType<VkPhysicalDevice>(boxed);
}

VulkanDispatch* dispatch_VkPhysicalDevice(VkPhysicalDevice boxed) {
    return get_dispatch_VkType<VkPhysicalDevice>(boxed);
}

VkDevice new_boxed_VkDevice(VkDevice unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
    return new_boxed_VkType<VkDevice>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
}

void delete_VkDevice(VkDevice boxed) {
    delete_VkType(boxed);
}

VkDevice unbox_VkDevice(VkDevice boxed) {
    return unbox_VkType<VkDevice>(boxed);
}

VkDevice try_unbox_VkDevice(VkDevice boxed) {
    return try_unbox_VkType<VkDevice>(boxed);
}

VkDevice unboxed_to_boxed_VkDevice(VkDevice unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDevice>(unboxed);
}

OrderMaintenanceInfo* ordmaint_VkDevice(VkDevice boxed) {
    return get_order_maintenance_info_VkType<VkDevice>(boxed);
}

VulkanMemReadingStream* readstream_VkDevice(VkDevice boxed) {
    return get_read_stream_VkType<VkDevice>(boxed);
}

VulkanDispatch* dispatch_VkDevice(VkDevice boxed) {
    return get_dispatch_VkType<VkDevice>(boxed);
}

VkCommandBuffer new_boxed_VkCommandBuffer(VkCommandBuffer unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
    return new_boxed_VkType<VkCommandBuffer>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
}

void delete_VkCommandBuffer(VkCommandBuffer boxed) {
    delete_VkType(boxed);
}

VkCommandBuffer unbox_VkCommandBuffer(VkCommandBuffer boxed) {
    return unbox_VkType<VkCommandBuffer>(boxed);
}

VkCommandBuffer try_unbox_VkCommandBuffer(VkCommandBuffer boxed) {
    return try_unbox_VkType<VkCommandBuffer>(boxed);
}

VkCommandBuffer unboxed_to_boxed_VkCommandBuffer(VkCommandBuffer unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCommandBuffer>(unboxed);
}

OrderMaintenanceInfo* ordmaint_VkCommandBuffer(VkCommandBuffer boxed) {
    return get_order_maintenance_info_VkType<VkCommandBuffer>(boxed);
}

VulkanMemReadingStream* readstream_VkCommandBuffer(VkCommandBuffer boxed) {
    return get_read_stream_VkType<VkCommandBuffer>(boxed);
}

VulkanDispatch* dispatch_VkCommandBuffer(VkCommandBuffer boxed) {
    return get_dispatch_VkType<VkCommandBuffer>(boxed);
}

VkQueue new_boxed_VkQueue(VkQueue unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
    return new_boxed_VkType<VkQueue>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
}

void delete_VkQueue(VkQueue boxed) {
    delete_VkType(boxed);
}

VkQueue unbox_VkQueue(VkQueue boxed) {
    return unbox_VkType<VkQueue>(boxed);
}

VkQueue try_unbox_VkQueue(VkQueue boxed) {
    return try_unbox_VkType<VkQueue>(boxed);
}

VkQueue unboxed_to_boxed_VkQueue(VkQueue unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkQueue>(unboxed);
}

OrderMaintenanceInfo* ordmaint_VkQueue(VkQueue boxed) {
    return get_order_maintenance_info_VkType<VkQueue>(boxed);
}

VulkanMemReadingStream* readstream_VkQueue(VkQueue boxed) {
    return get_read_stream_VkType<VkQueue>(boxed);
}

VulkanDispatch* dispatch_VkQueue(VkQueue boxed) {
    return get_dispatch_VkType<VkQueue>(boxed);
}

///////////////////////////////////////////////////////////////////////////////
//////////////             NON DISPATCHABLE TYPES                //////////////
///////////////////////////////////////////////////////////////////////////////

VkAccelerationStructureKHR new_boxed_non_dispatchable_VkAccelerationStructureKHR(VkAccelerationStructureKHR unboxed) {
    return new_boxed_VkType<VkAccelerationStructureKHR>(unboxed);
}

void delete_VkAccelerationStructureKHR(VkAccelerationStructureKHR boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkAccelerationStructureKHR(VkAccelerationStructureKHR boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkAccelerationStructureKHR unbox_VkAccelerationStructureKHR(VkAccelerationStructureKHR boxed) {
    return unbox_VkType<VkAccelerationStructureKHR>(boxed);
}

VkAccelerationStructureKHR try_unbox_VkAccelerationStructureKHR(VkAccelerationStructureKHR boxed) {
    return try_unbox_VkType<VkAccelerationStructureKHR>(boxed);
}

VkAccelerationStructureKHR unboxed_to_boxed_non_dispatchable_VkAccelerationStructureKHR(VkAccelerationStructureKHR unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkAccelerationStructureKHR>(unboxed);
}

void set_boxed_non_dispatchable_VkAccelerationStructureKHR(VkAccelerationStructureKHR boxed, VkAccelerationStructureKHR new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkAccelerationStructureKHR>(boxed, new_unboxed);
}

VkAccelerationStructureNV new_boxed_non_dispatchable_VkAccelerationStructureNV(VkAccelerationStructureNV unboxed) {
    return new_boxed_VkType<VkAccelerationStructureNV>(unboxed);
}

void delete_VkAccelerationStructureNV(VkAccelerationStructureNV boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkAccelerationStructureNV(VkAccelerationStructureNV boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkAccelerationStructureNV unbox_VkAccelerationStructureNV(VkAccelerationStructureNV boxed) {
    return unbox_VkType<VkAccelerationStructureNV>(boxed);
}

VkAccelerationStructureNV try_unbox_VkAccelerationStructureNV(VkAccelerationStructureNV boxed) {
    return try_unbox_VkType<VkAccelerationStructureNV>(boxed);
}

VkAccelerationStructureNV unboxed_to_boxed_non_dispatchable_VkAccelerationStructureNV(VkAccelerationStructureNV unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkAccelerationStructureNV>(unboxed);
}

void set_boxed_non_dispatchable_VkAccelerationStructureNV(VkAccelerationStructureNV boxed, VkAccelerationStructureNV new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkAccelerationStructureNV>(boxed, new_unboxed);
}

VkBuffer new_boxed_non_dispatchable_VkBuffer(VkBuffer unboxed) {
    return new_boxed_VkType<VkBuffer>(unboxed);
}

void delete_VkBuffer(VkBuffer boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkBuffer(VkBuffer boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkBuffer unbox_VkBuffer(VkBuffer boxed) {
    return unbox_VkType<VkBuffer>(boxed);
}

VkBuffer try_unbox_VkBuffer(VkBuffer boxed) {
    return try_unbox_VkType<VkBuffer>(boxed);
}

VkBuffer unboxed_to_boxed_non_dispatchable_VkBuffer(VkBuffer unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkBuffer>(unboxed);
}

void set_boxed_non_dispatchable_VkBuffer(VkBuffer boxed, VkBuffer new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkBuffer>(boxed, new_unboxed);
}

VkBufferView new_boxed_non_dispatchable_VkBufferView(VkBufferView unboxed) {
    return new_boxed_VkType<VkBufferView>(unboxed);
}

void delete_VkBufferView(VkBufferView boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkBufferView(VkBufferView boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkBufferView unbox_VkBufferView(VkBufferView boxed) {
    return unbox_VkType<VkBufferView>(boxed);
}

VkBufferView try_unbox_VkBufferView(VkBufferView boxed) {
    return try_unbox_VkType<VkBufferView>(boxed);
}

VkBufferView unboxed_to_boxed_non_dispatchable_VkBufferView(VkBufferView unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkBufferView>(unboxed);
}

void set_boxed_non_dispatchable_VkBufferView(VkBufferView boxed, VkBufferView new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkBufferView>(boxed, new_unboxed);
}

VkCommandPool new_boxed_non_dispatchable_VkCommandPool(VkCommandPool unboxed) {
    return new_boxed_VkType<VkCommandPool>(unboxed);
}

void delete_VkCommandPool(VkCommandPool boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkCommandPool(VkCommandPool boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkCommandPool unbox_VkCommandPool(VkCommandPool boxed) {
    return unbox_VkType<VkCommandPool>(boxed);
}

VkCommandPool try_unbox_VkCommandPool(VkCommandPool boxed) {
    return try_unbox_VkType<VkCommandPool>(boxed);
}

VkCommandPool unboxed_to_boxed_non_dispatchable_VkCommandPool(VkCommandPool unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCommandPool>(unboxed);
}

void set_boxed_non_dispatchable_VkCommandPool(VkCommandPool boxed, VkCommandPool new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkCommandPool>(boxed, new_unboxed);
}

VkCuFunctionNVX new_boxed_non_dispatchable_VkCuFunctionNVX(VkCuFunctionNVX unboxed) {
    return new_boxed_VkType<VkCuFunctionNVX>(unboxed);
}

void delete_VkCuFunctionNVX(VkCuFunctionNVX boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkCuFunctionNVX(VkCuFunctionNVX boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkCuFunctionNVX unbox_VkCuFunctionNVX(VkCuFunctionNVX boxed) {
    return unbox_VkType<VkCuFunctionNVX>(boxed);
}

VkCuFunctionNVX try_unbox_VkCuFunctionNVX(VkCuFunctionNVX boxed) {
    return try_unbox_VkType<VkCuFunctionNVX>(boxed);
}

VkCuFunctionNVX unboxed_to_boxed_non_dispatchable_VkCuFunctionNVX(VkCuFunctionNVX unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCuFunctionNVX>(unboxed);
}

void set_boxed_non_dispatchable_VkCuFunctionNVX(VkCuFunctionNVX boxed, VkCuFunctionNVX new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkCuFunctionNVX>(boxed, new_unboxed);
}

VkCuModuleNVX new_boxed_non_dispatchable_VkCuModuleNVX(VkCuModuleNVX unboxed) {
    return new_boxed_VkType<VkCuModuleNVX>(unboxed);
}

void delete_VkCuModuleNVX(VkCuModuleNVX boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkCuModuleNVX(VkCuModuleNVX boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkCuModuleNVX unbox_VkCuModuleNVX(VkCuModuleNVX boxed) {
    return unbox_VkType<VkCuModuleNVX>(boxed);
}

VkCuModuleNVX try_unbox_VkCuModuleNVX(VkCuModuleNVX boxed) {
    return try_unbox_VkType<VkCuModuleNVX>(boxed);
}

VkCuModuleNVX unboxed_to_boxed_non_dispatchable_VkCuModuleNVX(VkCuModuleNVX unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCuModuleNVX>(unboxed);
}

void set_boxed_non_dispatchable_VkCuModuleNVX(VkCuModuleNVX boxed, VkCuModuleNVX new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkCuModuleNVX>(boxed, new_unboxed);
}

VkDebugReportCallbackEXT new_boxed_non_dispatchable_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT unboxed) {
    return new_boxed_VkType<VkDebugReportCallbackEXT>(unboxed);
}

void delete_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDebugReportCallbackEXT unbox_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT boxed) {
    return unbox_VkType<VkDebugReportCallbackEXT>(boxed);
}

VkDebugReportCallbackEXT try_unbox_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT boxed) {
    return try_unbox_VkType<VkDebugReportCallbackEXT>(boxed);
}

VkDebugReportCallbackEXT unboxed_to_boxed_non_dispatchable_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDebugReportCallbackEXT>(unboxed);
}

void set_boxed_non_dispatchable_VkDebugReportCallbackEXT(VkDebugReportCallbackEXT boxed, VkDebugReportCallbackEXT new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDebugReportCallbackEXT>(boxed, new_unboxed);
}

VkDebugUtilsMessengerEXT new_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT unboxed) {
    return new_boxed_VkType<VkDebugUtilsMessengerEXT>(unboxed);
}

void delete_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDebugUtilsMessengerEXT unbox_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT boxed) {
    return unbox_VkType<VkDebugUtilsMessengerEXT>(boxed);
}

VkDebugUtilsMessengerEXT try_unbox_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT boxed) {
    return try_unbox_VkType<VkDebugUtilsMessengerEXT>(boxed);
}

VkDebugUtilsMessengerEXT unboxed_to_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDebugUtilsMessengerEXT>(unboxed);
}

void set_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT boxed, VkDebugUtilsMessengerEXT new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDebugUtilsMessengerEXT>(boxed, new_unboxed);
}

VkDescriptorPool new_boxed_non_dispatchable_VkDescriptorPool(VkDescriptorPool unboxed) {
    return new_boxed_VkType<VkDescriptorPool>(unboxed);
}

void delete_VkDescriptorPool(VkDescriptorPool boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDescriptorPool(VkDescriptorPool boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDescriptorPool unbox_VkDescriptorPool(VkDescriptorPool boxed) {
    return unbox_VkType<VkDescriptorPool>(boxed);
}

VkDescriptorPool try_unbox_VkDescriptorPool(VkDescriptorPool boxed) {
    return try_unbox_VkType<VkDescriptorPool>(boxed);
}

VkDescriptorPool unboxed_to_boxed_non_dispatchable_VkDescriptorPool(VkDescriptorPool unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorPool>(unboxed);
}

void set_boxed_non_dispatchable_VkDescriptorPool(VkDescriptorPool boxed, VkDescriptorPool new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDescriptorPool>(boxed, new_unboxed);
}

VkDescriptorSet new_boxed_non_dispatchable_VkDescriptorSet(VkDescriptorSet unboxed) {
    return new_boxed_VkType<VkDescriptorSet>(unboxed);
}

void delete_VkDescriptorSet(VkDescriptorSet boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDescriptorSet(VkDescriptorSet boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDescriptorSet unbox_VkDescriptorSet(VkDescriptorSet boxed) {
    return unbox_VkType<VkDescriptorSet>(boxed);
}

VkDescriptorSet try_unbox_VkDescriptorSet(VkDescriptorSet boxed) {
    return try_unbox_VkType<VkDescriptorSet>(boxed);
}

VkDescriptorSet unboxed_to_boxed_non_dispatchable_VkDescriptorSet(VkDescriptorSet unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorSet>(unboxed);
}

void set_boxed_non_dispatchable_VkDescriptorSet(VkDescriptorSet boxed, VkDescriptorSet new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDescriptorSet>(boxed, new_unboxed);
}

VkDescriptorSetLayout new_boxed_non_dispatchable_VkDescriptorSetLayout(VkDescriptorSetLayout unboxed) {
    return new_boxed_VkType<VkDescriptorSetLayout>(unboxed);
}

void delete_VkDescriptorSetLayout(VkDescriptorSetLayout boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDescriptorSetLayout(VkDescriptorSetLayout boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDescriptorSetLayout unbox_VkDescriptorSetLayout(VkDescriptorSetLayout boxed) {
    return unbox_VkType<VkDescriptorSetLayout>(boxed);
}

VkDescriptorSetLayout try_unbox_VkDescriptorSetLayout(VkDescriptorSetLayout boxed) {
    return try_unbox_VkType<VkDescriptorSetLayout>(boxed);
}

VkDescriptorSetLayout unboxed_to_boxed_non_dispatchable_VkDescriptorSetLayout(VkDescriptorSetLayout unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorSetLayout>(unboxed);
}

void set_boxed_non_dispatchable_VkDescriptorSetLayout(VkDescriptorSetLayout boxed, VkDescriptorSetLayout new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDescriptorSetLayout>(boxed, new_unboxed);
}

VkDescriptorUpdateTemplate new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate unboxed) {
    return new_boxed_VkType<VkDescriptorUpdateTemplate>(unboxed);
}

void delete_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDescriptorUpdateTemplate unbox_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate boxed) {
    return unbox_VkType<VkDescriptorUpdateTemplate>(boxed);
}

VkDescriptorUpdateTemplate try_unbox_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate boxed) {
    return try_unbox_VkType<VkDescriptorUpdateTemplate>(boxed);
}

VkDescriptorUpdateTemplate unboxed_to_boxed_non_dispatchable_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorUpdateTemplate>(unboxed);
}

void set_boxed_non_dispatchable_VkDescriptorUpdateTemplate(VkDescriptorUpdateTemplate boxed, VkDescriptorUpdateTemplate new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDescriptorUpdateTemplate>(boxed, new_unboxed);
}

VkDeviceMemory new_boxed_non_dispatchable_VkDeviceMemory(VkDeviceMemory unboxed) {
    return new_boxed_VkType<VkDeviceMemory>(unboxed);
}

void delete_VkDeviceMemory(VkDeviceMemory boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDeviceMemory(VkDeviceMemory boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDeviceMemory unbox_VkDeviceMemory(VkDeviceMemory boxed) {
    return unbox_VkType<VkDeviceMemory>(boxed);
}

VkDeviceMemory try_unbox_VkDeviceMemory(VkDeviceMemory boxed) {
    return try_unbox_VkType<VkDeviceMemory>(boxed);
}

VkDeviceMemory unboxed_to_boxed_non_dispatchable_VkDeviceMemory(VkDeviceMemory unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDeviceMemory>(unboxed);
}

void set_boxed_non_dispatchable_VkDeviceMemory(VkDeviceMemory boxed, VkDeviceMemory new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDeviceMemory>(boxed, new_unboxed);
}

VkDisplayKHR new_boxed_non_dispatchable_VkDisplayKHR(VkDisplayKHR unboxed) {
    return new_boxed_VkType<VkDisplayKHR>(unboxed);
}

void delete_VkDisplayKHR(VkDisplayKHR boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDisplayKHR(VkDisplayKHR boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDisplayKHR unbox_VkDisplayKHR(VkDisplayKHR boxed) {
    return unbox_VkType<VkDisplayKHR>(boxed);
}

VkDisplayKHR try_unbox_VkDisplayKHR(VkDisplayKHR boxed) {
    return try_unbox_VkType<VkDisplayKHR>(boxed);
}

VkDisplayKHR unboxed_to_boxed_non_dispatchable_VkDisplayKHR(VkDisplayKHR unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDisplayKHR>(unboxed);
}

void set_boxed_non_dispatchable_VkDisplayKHR(VkDisplayKHR boxed, VkDisplayKHR new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDisplayKHR>(boxed, new_unboxed);
}

VkDisplayModeKHR new_boxed_non_dispatchable_VkDisplayModeKHR(VkDisplayModeKHR unboxed) {
    return new_boxed_VkType<VkDisplayModeKHR>(unboxed);
}

void delete_VkDisplayModeKHR(VkDisplayModeKHR boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkDisplayModeKHR(VkDisplayModeKHR boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkDisplayModeKHR unbox_VkDisplayModeKHR(VkDisplayModeKHR boxed) {
    return unbox_VkType<VkDisplayModeKHR>(boxed);
}

VkDisplayModeKHR try_unbox_VkDisplayModeKHR(VkDisplayModeKHR boxed) {
    return try_unbox_VkType<VkDisplayModeKHR>(boxed);
}

VkDisplayModeKHR unboxed_to_boxed_non_dispatchable_VkDisplayModeKHR(VkDisplayModeKHR unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDisplayModeKHR>(unboxed);
}

void set_boxed_non_dispatchable_VkDisplayModeKHR(VkDisplayModeKHR boxed, VkDisplayModeKHR new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkDisplayModeKHR>(boxed, new_unboxed);
}

VkEvent new_boxed_non_dispatchable_VkEvent(VkEvent unboxed) {
    return new_boxed_VkType<VkEvent>(unboxed);
}

void delete_VkEvent(VkEvent boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkEvent(VkEvent boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkEvent unbox_VkEvent(VkEvent boxed) {
    return unbox_VkType<VkEvent>(boxed);
}

VkEvent try_unbox_VkEvent(VkEvent boxed) {
    return try_unbox_VkType<VkEvent>(boxed);
}

VkEvent unboxed_to_boxed_non_dispatchable_VkEvent(VkEvent unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkEvent>(unboxed);
}

void set_boxed_non_dispatchable_VkEvent(VkEvent boxed, VkEvent new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkEvent>(boxed, new_unboxed);
}

VkFence new_boxed_non_dispatchable_VkFence(VkFence unboxed) {
    return new_boxed_VkType<VkFence>(unboxed);
}

void delete_VkFence(VkFence boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkFence(VkFence boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkFence unbox_VkFence(VkFence boxed) {
    return unbox_VkType<VkFence>(boxed);
}

VkFence try_unbox_VkFence(VkFence boxed) {
    return try_unbox_VkType<VkFence>(boxed);
}

VkFence unboxed_to_boxed_non_dispatchable_VkFence(VkFence unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkFence>(unboxed);
}

void set_boxed_non_dispatchable_VkFence(VkFence boxed, VkFence new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkFence>(boxed, new_unboxed);
}

VkFramebuffer new_boxed_non_dispatchable_VkFramebuffer(VkFramebuffer unboxed) {
    return new_boxed_VkType<VkFramebuffer>(unboxed);
}

void delete_VkFramebuffer(VkFramebuffer boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkFramebuffer(VkFramebuffer boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkFramebuffer unbox_VkFramebuffer(VkFramebuffer boxed) {
    return unbox_VkType<VkFramebuffer>(boxed);
}

VkFramebuffer try_unbox_VkFramebuffer(VkFramebuffer boxed) {
    return try_unbox_VkType<VkFramebuffer>(boxed);
}

VkFramebuffer unboxed_to_boxed_non_dispatchable_VkFramebuffer(VkFramebuffer unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkFramebuffer>(unboxed);
}

void set_boxed_non_dispatchable_VkFramebuffer(VkFramebuffer boxed, VkFramebuffer new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkFramebuffer>(boxed, new_unboxed);
}

VkImage new_boxed_non_dispatchable_VkImage(VkImage unboxed) {
    return new_boxed_VkType<VkImage>(unboxed);
}

void delete_VkImage(VkImage boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkImage(VkImage boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkImage unbox_VkImage(VkImage boxed) {
    return unbox_VkType<VkImage>(boxed);
}

VkImage try_unbox_VkImage(VkImage boxed) {
    return try_unbox_VkType<VkImage>(boxed);
}

VkImage unboxed_to_boxed_non_dispatchable_VkImage(VkImage unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkImage>(unboxed);
}

void set_boxed_non_dispatchable_VkImage(VkImage boxed, VkImage new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkImage>(boxed, new_unboxed);
}

VkImageView new_boxed_non_dispatchable_VkImageView(VkImageView unboxed) {
    return new_boxed_VkType<VkImageView>(unboxed);
}

void delete_VkImageView(VkImageView boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkImageView(VkImageView boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkImageView unbox_VkImageView(VkImageView boxed) {
    return unbox_VkType<VkImageView>(boxed);
}

VkImageView try_unbox_VkImageView(VkImageView boxed) {
    return try_unbox_VkType<VkImageView>(boxed);
}

VkImageView unboxed_to_boxed_non_dispatchable_VkImageView(VkImageView unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkImageView>(unboxed);
}

void set_boxed_non_dispatchable_VkImageView(VkImageView boxed, VkImageView new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkImageView>(boxed, new_unboxed);
}

VkIndirectCommandsLayoutNV new_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV unboxed) {
    return new_boxed_VkType<VkIndirectCommandsLayoutNV>(unboxed);
}

void delete_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkIndirectCommandsLayoutNV unbox_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV boxed) {
    return unbox_VkType<VkIndirectCommandsLayoutNV>(boxed);
}

VkIndirectCommandsLayoutNV try_unbox_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV boxed) {
    return try_unbox_VkType<VkIndirectCommandsLayoutNV>(boxed);
}

VkIndirectCommandsLayoutNV unboxed_to_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkIndirectCommandsLayoutNV>(unboxed);
}

void set_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(VkIndirectCommandsLayoutNV boxed, VkIndirectCommandsLayoutNV new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkIndirectCommandsLayoutNV>(boxed, new_unboxed);
}

VkMicromapEXT new_boxed_non_dispatchable_VkMicromapEXT(VkMicromapEXT unboxed) {
    return new_boxed_VkType<VkMicromapEXT>(unboxed);
}

void delete_VkMicromapEXT(VkMicromapEXT boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkMicromapEXT(VkMicromapEXT boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkMicromapEXT unbox_VkMicromapEXT(VkMicromapEXT boxed) {
    return unbox_VkType<VkMicromapEXT>(boxed);
}

VkMicromapEXT try_unbox_VkMicromapEXT(VkMicromapEXT boxed) {
    return try_unbox_VkType<VkMicromapEXT>(boxed);
}

VkMicromapEXT unboxed_to_boxed_non_dispatchable_VkMicromapEXT(VkMicromapEXT unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkMicromapEXT>(unboxed);
}

void set_boxed_non_dispatchable_VkMicromapEXT(VkMicromapEXT boxed, VkMicromapEXT new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkMicromapEXT>(boxed, new_unboxed);
}

VkPipeline new_boxed_non_dispatchable_VkPipeline(VkPipeline unboxed) {
    return new_boxed_VkType<VkPipeline>(unboxed);
}

void delete_VkPipeline(VkPipeline boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkPipeline(VkPipeline boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkPipeline unbox_VkPipeline(VkPipeline boxed) {
    return unbox_VkType<VkPipeline>(boxed);
}

VkPipeline try_unbox_VkPipeline(VkPipeline boxed) {
    return try_unbox_VkType<VkPipeline>(boxed);
}

VkPipeline unboxed_to_boxed_non_dispatchable_VkPipeline(VkPipeline unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipeline>(unboxed);
}

void set_boxed_non_dispatchable_VkPipeline(VkPipeline boxed, VkPipeline new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkPipeline>(boxed, new_unboxed);
}

VkPipelineCache new_boxed_non_dispatchable_VkPipelineCache(VkPipelineCache unboxed) {
    return new_boxed_VkType<VkPipelineCache>(unboxed);
}

void delete_VkPipelineCache(VkPipelineCache boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkPipelineCache(VkPipelineCache boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkPipelineCache unbox_VkPipelineCache(VkPipelineCache boxed) {
    return unbox_VkType<VkPipelineCache>(boxed);
}

VkPipelineCache try_unbox_VkPipelineCache(VkPipelineCache boxed) {
    return try_unbox_VkType<VkPipelineCache>(boxed);
}

VkPipelineCache unboxed_to_boxed_non_dispatchable_VkPipelineCache(VkPipelineCache unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipelineCache>(unboxed);
}

void set_boxed_non_dispatchable_VkPipelineCache(VkPipelineCache boxed, VkPipelineCache new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkPipelineCache>(boxed, new_unboxed);
}

VkPipelineLayout new_boxed_non_dispatchable_VkPipelineLayout(VkPipelineLayout unboxed) {
    return new_boxed_VkType<VkPipelineLayout>(unboxed);
}

void delete_VkPipelineLayout(VkPipelineLayout boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkPipelineLayout(VkPipelineLayout boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkPipelineLayout unbox_VkPipelineLayout(VkPipelineLayout boxed) {
    return unbox_VkType<VkPipelineLayout>(boxed);
}

VkPipelineLayout try_unbox_VkPipelineLayout(VkPipelineLayout boxed) {
    return try_unbox_VkType<VkPipelineLayout>(boxed);
}

VkPipelineLayout unboxed_to_boxed_non_dispatchable_VkPipelineLayout(VkPipelineLayout unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipelineLayout>(unboxed);
}

void set_boxed_non_dispatchable_VkPipelineLayout(VkPipelineLayout boxed, VkPipelineLayout new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkPipelineLayout>(boxed, new_unboxed);
}

VkPrivateDataSlot new_boxed_non_dispatchable_VkPrivateDataSlot(VkPrivateDataSlot unboxed) {
    return new_boxed_VkType<VkPrivateDataSlot>(unboxed);
}

void delete_VkPrivateDataSlot(VkPrivateDataSlot boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkPrivateDataSlot(VkPrivateDataSlot boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkPrivateDataSlot unbox_VkPrivateDataSlot(VkPrivateDataSlot boxed) {
    return unbox_VkType<VkPrivateDataSlot>(boxed);
}

VkPrivateDataSlot try_unbox_VkPrivateDataSlot(VkPrivateDataSlot boxed) {
    return try_unbox_VkType<VkPrivateDataSlot>(boxed);
}

VkPrivateDataSlot unboxed_to_boxed_non_dispatchable_VkPrivateDataSlot(VkPrivateDataSlot unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPrivateDataSlot>(unboxed);
}

void set_boxed_non_dispatchable_VkPrivateDataSlot(VkPrivateDataSlot boxed, VkPrivateDataSlot new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkPrivateDataSlot>(boxed, new_unboxed);
}

VkQueryPool new_boxed_non_dispatchable_VkQueryPool(VkQueryPool unboxed) {
    return new_boxed_VkType<VkQueryPool>(unboxed);
}

void delete_VkQueryPool(VkQueryPool boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkQueryPool(VkQueryPool boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkQueryPool unbox_VkQueryPool(VkQueryPool boxed) {
    return unbox_VkType<VkQueryPool>(boxed);
}

VkQueryPool try_unbox_VkQueryPool(VkQueryPool boxed) {
    return try_unbox_VkType<VkQueryPool>(boxed);
}

VkQueryPool unboxed_to_boxed_non_dispatchable_VkQueryPool(VkQueryPool unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkQueryPool>(unboxed);
}

void set_boxed_non_dispatchable_VkQueryPool(VkQueryPool boxed, VkQueryPool new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkQueryPool>(boxed, new_unboxed);
}

VkRenderPass new_boxed_non_dispatchable_VkRenderPass(VkRenderPass unboxed) {
    return new_boxed_VkType<VkRenderPass>(unboxed);
}

void delete_VkRenderPass(VkRenderPass boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkRenderPass(VkRenderPass boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkRenderPass unbox_VkRenderPass(VkRenderPass boxed) {
    return unbox_VkType<VkRenderPass>(boxed);
}

VkRenderPass try_unbox_VkRenderPass(VkRenderPass boxed) {
    return try_unbox_VkType<VkRenderPass>(boxed);
}

VkRenderPass unboxed_to_boxed_non_dispatchable_VkRenderPass(VkRenderPass unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkRenderPass>(unboxed);
}

void set_boxed_non_dispatchable_VkRenderPass(VkRenderPass boxed, VkRenderPass new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkRenderPass>(boxed, new_unboxed);
}

VkSampler new_boxed_non_dispatchable_VkSampler(VkSampler unboxed) {
    return new_boxed_VkType<VkSampler>(unboxed);
}

void delete_VkSampler(VkSampler boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkSampler(VkSampler boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkSampler unbox_VkSampler(VkSampler boxed) {
    return unbox_VkType<VkSampler>(boxed);
}

VkSampler try_unbox_VkSampler(VkSampler boxed) {
    return try_unbox_VkType<VkSampler>(boxed);
}

VkSampler unboxed_to_boxed_non_dispatchable_VkSampler(VkSampler unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSampler>(unboxed);
}

void set_boxed_non_dispatchable_VkSampler(VkSampler boxed, VkSampler new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkSampler>(boxed, new_unboxed);
}

VkSamplerYcbcrConversion new_boxed_non_dispatchable_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion unboxed) {
    return new_boxed_VkType<VkSamplerYcbcrConversion>(unboxed);
}

void delete_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkSamplerYcbcrConversion unbox_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion boxed) {
    return unbox_VkType<VkSamplerYcbcrConversion>(boxed);
}

VkSamplerYcbcrConversion try_unbox_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion boxed) {
    return try_unbox_VkType<VkSamplerYcbcrConversion>(boxed);
}

VkSamplerYcbcrConversion unboxed_to_boxed_non_dispatchable_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSamplerYcbcrConversion>(unboxed);
}

void set_boxed_non_dispatchable_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion boxed, VkSamplerYcbcrConversion new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkSamplerYcbcrConversion>(boxed, new_unboxed);
}

VkSemaphore new_boxed_non_dispatchable_VkSemaphore(VkSemaphore unboxed) {
    return new_boxed_VkType<VkSemaphore>(unboxed);
}

void delete_VkSemaphore(VkSemaphore boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkSemaphore(VkSemaphore boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkSemaphore unbox_VkSemaphore(VkSemaphore boxed) {
    return unbox_VkType<VkSemaphore>(boxed);
}

VkSemaphore try_unbox_VkSemaphore(VkSemaphore boxed) {
    return try_unbox_VkType<VkSemaphore>(boxed);
}

VkSemaphore unboxed_to_boxed_non_dispatchable_VkSemaphore(VkSemaphore unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSemaphore>(unboxed);
}

void set_boxed_non_dispatchable_VkSemaphore(VkSemaphore boxed, VkSemaphore new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkSemaphore>(boxed, new_unboxed);
}

VkShaderModule new_boxed_non_dispatchable_VkShaderModule(VkShaderModule unboxed) {
    return new_boxed_VkType<VkShaderModule>(unboxed);
}

void delete_VkShaderModule(VkShaderModule boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkShaderModule(VkShaderModule boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkShaderModule unbox_VkShaderModule(VkShaderModule boxed) {
    return unbox_VkType<VkShaderModule>(boxed);
}

VkShaderModule try_unbox_VkShaderModule(VkShaderModule boxed) {
    return try_unbox_VkType<VkShaderModule>(boxed);
}

VkShaderModule unboxed_to_boxed_non_dispatchable_VkShaderModule(VkShaderModule unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkShaderModule>(unboxed);
}

void set_boxed_non_dispatchable_VkShaderModule(VkShaderModule boxed, VkShaderModule new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkShaderModule>(boxed, new_unboxed);
}

VkSurfaceKHR new_boxed_non_dispatchable_VkSurfaceKHR(VkSurfaceKHR unboxed) {
    return new_boxed_VkType<VkSurfaceKHR>(unboxed);
}

void delete_VkSurfaceKHR(VkSurfaceKHR boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkSurfaceKHR(VkSurfaceKHR boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkSurfaceKHR unbox_VkSurfaceKHR(VkSurfaceKHR boxed) {
    return unbox_VkType<VkSurfaceKHR>(boxed);
}

VkSurfaceKHR try_unbox_VkSurfaceKHR(VkSurfaceKHR boxed) {
    return try_unbox_VkType<VkSurfaceKHR>(boxed);
}

VkSurfaceKHR unboxed_to_boxed_non_dispatchable_VkSurfaceKHR(VkSurfaceKHR unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSurfaceKHR>(unboxed);
}

void set_boxed_non_dispatchable_VkSurfaceKHR(VkSurfaceKHR boxed, VkSurfaceKHR new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkSurfaceKHR>(boxed, new_unboxed);
}

VkSwapchainKHR new_boxed_non_dispatchable_VkSwapchainKHR(VkSwapchainKHR unboxed) {
    return new_boxed_VkType<VkSwapchainKHR>(unboxed);
}

void delete_VkSwapchainKHR(VkSwapchainKHR boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkSwapchainKHR(VkSwapchainKHR boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkSwapchainKHR unbox_VkSwapchainKHR(VkSwapchainKHR boxed) {
    return unbox_VkType<VkSwapchainKHR>(boxed);
}

VkSwapchainKHR try_unbox_VkSwapchainKHR(VkSwapchainKHR boxed) {
    return try_unbox_VkType<VkSwapchainKHR>(boxed);
}

VkSwapchainKHR unboxed_to_boxed_non_dispatchable_VkSwapchainKHR(VkSwapchainKHR unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSwapchainKHR>(unboxed);
}

void set_boxed_non_dispatchable_VkSwapchainKHR(VkSwapchainKHR boxed, VkSwapchainKHR new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkSwapchainKHR>(boxed, new_unboxed);
}

VkValidationCacheEXT new_boxed_non_dispatchable_VkValidationCacheEXT(VkValidationCacheEXT unboxed) {
    return new_boxed_VkType<VkValidationCacheEXT>(unboxed);
}

void delete_VkValidationCacheEXT(VkValidationCacheEXT boxed) {
    delete_VkType(boxed);
}

void delayed_delete_VkValidationCacheEXT(VkValidationCacheEXT boxed, VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(boxed, device, std::move(callback));
}

VkValidationCacheEXT unbox_VkValidationCacheEXT(VkValidationCacheEXT boxed) {
    return unbox_VkType<VkValidationCacheEXT>(boxed);
}

VkValidationCacheEXT try_unbox_VkValidationCacheEXT(VkValidationCacheEXT boxed) {
    return try_unbox_VkType<VkValidationCacheEXT>(boxed);
}

VkValidationCacheEXT unboxed_to_boxed_non_dispatchable_VkValidationCacheEXT(VkValidationCacheEXT unboxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkValidationCacheEXT>(unboxed);
}

void set_boxed_non_dispatchable_VkValidationCacheEXT(VkValidationCacheEXT boxed, VkValidationCacheEXT new_unboxed) {
    set_boxed_non_dispatchable_VkType<VkValidationCacheEXT>(boxed, new_unboxed);
}

//VkInstance new_boxed_VkInstance(VkInstance unboxed, VulkanDispatch* dispatch, bool ownsDispatch) {
//    return new_boxed_VkType<VkInstance>(unboxed, /*dispatchable=*/true, dispatch, ownsDispatch);
// }
VkInstance new_boxed_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance unboxed, VkInstance underlying, VulkanDispatch* dispatch, bool ownDispatch) {
    return new_boxed_VkType<VkInstance>(pBoxedHandleManager, unboxed, true, dispatch, ownDispatch);
}

#if 0
void delete_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
VkInstance unbox_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance boxed) {
    return unbox_VkType<VkInstance>(pBoxedHandleManager, boxed);
}
VkInstance try_unbox_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance boxed) {
    return try_unbox_VkType<VkInstance>(pBoxedHandleManager, boxed);
}
VkInstance unboxed_to_boxed_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkInstance>(pBoxedHandleManager, unboxed);
}
VulkanDispatch* dispatch_VkInstance(BoxedHandleManager* pBoxedHandleManager, VkInstance boxed) {
    return get_dispatch_VkType<VkInstance>(pBoxedHandleManager, boxed);
}
OrderMaintenanceInfo* ordmaint_VkInstance(BoxedHandleManager* pBoxedHandleManager,
                                          VkInstance boxed);
return get_order_maintenance_info_VkType<VkInstance>(pBoxedHandleManager, boxed);
}
VulkanMemReadingStream* readstream_VkInstance(BoxedHandleManager* pBoxedHandleManager,
                                              VkInstance boxed) {
    return get_read_stream_VkType<VkInstance>(pBoxedHandleManager, boxed);
}
VkPhysicalDevice new_boxed_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                            VkPhysicalDevice underlying, VulkanDispatch* dispatch,
                                            bool ownDispatch) {
    return new_boxed_VkType<VkPhysicalDevice>(pBoxedHandleManager, unboxed);
}
void delete_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager, VkPhysicalDevice boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
VkPhysicalDevice unbox_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                        VkPhysicalDevice boxed) {
    return unbox_VkType<VkPhysicalDevice>(pBoxedHandleManager, boxed);
}
VkPhysicalDevice try_unbox_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                            VkPhysicalDevice boxed) {
    return try_unbox_VkType<VkPhysicalDevice>(pBoxedHandleManager, boxed);
}
VkPhysicalDevice unboxed_to_boxed_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                                   VkPhysicalDevice boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPhysicalDevice>(pBoxedHandleManager, unboxed);
}
VulkanDispatch* dispatch_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                          VkPhysicalDevice boxed) {
    return get_dispatch_VkType<VkPhysicalDevice>(pBoxedHandleManager, boxed);
}
OrderMaintenanceInfo* ordmaint_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                                VkPhysicalDevice boxed);
return get_order_maintenance_info_VkType<VkPhysicalDevice>(pBoxedHandleManager, boxed);
}
VulkanMemReadingStream* readstream_VkPhysicalDevice(BoxedHandleManager* pBoxedHandleManager,
                                                    VkPhysicalDevice boxed) {
    return get_read_stream_VkType<VkPhysicalDevice>(pBoxedHandleManager, boxed);
}
VkDevice new_boxed_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice underlying,
                            VulkanDispatch* dispatch, bool ownDispatch) {
    return new_boxed_VkType<VkDevice>(pBoxedHandleManager, unboxed);
}
void delete_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
VkDevice unbox_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed) {
    return unbox_VkType<VkDevice>(pBoxedHandleManager, boxed);
}
VkDevice try_unbox_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed) {
    return try_unbox_VkType<VkDevice>(pBoxedHandleManager, boxed);
}
VkDevice unboxed_to_boxed_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDevice>(pBoxedHandleManager, unboxed);
}
VulkanDispatch* dispatch_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed) {
    return get_dispatch_VkType<VkDevice>(pBoxedHandleManager, boxed);
}
OrderMaintenanceInfo* ordmaint_VkDevice(BoxedHandleManager* pBoxedHandleManager, VkDevice boxed);
return get_order_maintenance_info_VkType<VkDevice>(pBoxedHandleManager, boxed);
}
VulkanMemReadingStream* readstream_VkDevice(BoxedHandleManager* pBoxedHandleManager,
                                            VkDevice boxed) {
    return get_read_stream_VkType<VkDevice>(pBoxedHandleManager, boxed);
}
VkQueue new_boxed_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue underlying,
                          VulkanDispatch* dispatch, bool ownDispatch) {
    return new_boxed_VkType<VkQueue>(pBoxedHandleManager, unboxed);
}
void delete_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
VkQueue unbox_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    return unbox_VkType<VkQueue>(pBoxedHandleManager, boxed);
}
VkQueue try_unbox_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    return try_unbox_VkType<VkQueue>(pBoxedHandleManager, boxed);
}
VkQueue unboxed_to_boxed_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkQueue>(pBoxedHandleManager, unboxed);
}
VulkanDispatch* dispatch_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    return get_dispatch_VkType<VkQueue>(pBoxedHandleManager, boxed);
}
OrderMaintenanceInfo* ordmaint_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed);
return get_order_maintenance_info_VkType<VkQueue>(pBoxedHandleManager, boxed);
}
VulkanMemReadingStream* readstream_VkQueue(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    return get_read_stream_VkType<VkQueue>(pBoxedHandleManager, boxed);
}
VkCommandBuffer new_boxed_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                          VkCommandBuffer underlying, VulkanDispatch* dispatch,
                                          bool ownDispatch) {
    return new_boxed_VkType<VkCommandBuffer>(pBoxedHandleManager, unboxed);
}
void delete_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager, VkCommandBuffer boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
VkCommandBuffer unbox_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                      VkCommandBuffer boxed) {
    return unbox_VkType<VkCommandBuffer>(pBoxedHandleManager, boxed);
}
VkCommandBuffer try_unbox_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                          VkCommandBuffer boxed) {
    return try_unbox_VkType<VkCommandBuffer>(pBoxedHandleManager, boxed);
}
VkCommandBuffer unboxed_to_boxed_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                                 VkCommandBuffer boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCommandBuffer>(pBoxedHandleManager, unboxed);
}
VulkanDispatch* dispatch_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                         VkCommandBuffer boxed) {
    return get_dispatch_VkType<VkCommandBuffer>(pBoxedHandleManager, boxed);
}
OrderMaintenanceInfo* ordmaint_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                               VkCommandBuffer boxed);
return get_order_maintenance_info_VkType<VkCommandBuffer>(pBoxedHandleManager, boxed);
}
VulkanMemReadingStream* readstream_VkCommandBuffer(BoxedHandleManager* pBoxedHandleManager,
                                                   VkCommandBuffer boxed) {
    return get_read_stream_VkType<VkCommandBuffer>(pBoxedHandleManager, boxed);
}
VkDeviceMemory new_boxed_non_dispatchable_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager,
                                                         VkDeviceMemory underlying) {
    return new_boxed_VkType<VkDeviceMemory>(pBoxedHandleManager, unboxed);
}
void delete_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager, VkDeviceMemory boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager, VkDeviceMemory boxed,
                                   VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDeviceMemory unbox_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager, VkDeviceMemory boxed) {
    return try_unbox_VkType<VkDeviceMemory>(pBoxedHandleManager, boxed);
}
VkDeviceMemory try_unbox_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager,
                                        VkDeviceMemory boxed) {
    return try_unbox_VkType<VkDeviceMemory>(pBoxedHandleManager, boxed);
}
VkDeviceMemory unboxed_to_boxed_non_dispatchable_VkDeviceMemory(
    BoxedHandleManager* pBoxedHandleManager, VkDeviceMemory boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDeviceMemory>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkDeviceMemory(BoxedHandleManager* pBoxedHandleManager,
                                               VkDeviceMemory boxed, VkDeviceMemory underlying) {
    set_boxed_non_dispatchable_VkType<VkDeviceMemory>(pBoxedHandleManager, boxed, new_unboxed);
}
VkBuffer new_boxed_non_dispatchable_VkBuffer(BoxedHandleManager* pBoxedHandleManager,
                                             VkBuffer underlying) {
    return new_boxed_VkType<VkBuffer>(pBoxedHandleManager, unboxed);
}
void delete_VkBuffer(BoxedHandleManager* pBoxedHandleManager, VkBuffer boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkBuffer(BoxedHandleManager* pBoxedHandleManager, VkBuffer boxed,
                             VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkBuffer unbox_VkBuffer(BoxedHandleManager* pBoxedHandleManager, VkBuffer boxed) {
    return try_unbox_VkType<VkBuffer>(pBoxedHandleManager, boxed);
}
VkBuffer try_unbox_VkBuffer(BoxedHandleManager* pBoxedHandleManager, VkBuffer boxed) {
    return try_unbox_VkType<VkBuffer>(pBoxedHandleManager, boxed);
}
VkBuffer unboxed_to_boxed_non_dispatchable_VkBuffer(BoxedHandleManager* pBoxedHandleManager,
                                                    VkBuffer boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkBuffer>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkBuffer(BoxedHandleManager* pBoxedHandleManager, VkBuffer boxed,
                                         VkBuffer underlying) {
    set_boxed_non_dispatchable_VkType<VkBuffer>(pBoxedHandleManager, boxed, new_unboxed);
}
VkBufferView new_boxed_non_dispatchable_VkBufferView(BoxedHandleManager* pBoxedHandleManager,
                                                     VkBufferView underlying) {
    return new_boxed_VkType<VkBufferView>(pBoxedHandleManager, unboxed);
}
void delete_VkBufferView(BoxedHandleManager* pBoxedHandleManager, VkBufferView boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkBufferView(BoxedHandleManager* pBoxedHandleManager, VkBufferView boxed,
                                 VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkBufferView unbox_VkBufferView(BoxedHandleManager* pBoxedHandleManager, VkBufferView boxed) {
    return try_unbox_VkType<VkBufferView>(pBoxedHandleManager, boxed);
}
VkBufferView try_unbox_VkBufferView(BoxedHandleManager* pBoxedHandleManager, VkBufferView boxed) {
    return try_unbox_VkType<VkBufferView>(pBoxedHandleManager, boxed);
}
VkBufferView unboxed_to_boxed_non_dispatchable_VkBufferView(BoxedHandleManager* pBoxedHandleManager,
                                                            VkBufferView boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkBufferView>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkBufferView(BoxedHandleManager* pBoxedHandleManager,
                                             VkBufferView boxed, VkBufferView underlying) {
    set_boxed_non_dispatchable_VkType<VkBufferView>(pBoxedHandleManager, boxed, new_unboxed);
}
VkImage new_boxed_non_dispatchable_VkImage(BoxedHandleManager* pBoxedHandleManager,
                                           VkImage underlying) {
    return new_boxed_VkType<VkImage>(pBoxedHandleManager, unboxed);
}
void delete_VkImage(BoxedHandleManager* pBoxedHandleManager, VkImage boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkImage(BoxedHandleManager* pBoxedHandleManager, VkImage boxed, VkDevice device,
                            std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkImage unbox_VkImage(BoxedHandleManager* pBoxedHandleManager, VkImage boxed) {
    return try_unbox_VkType<VkImage>(pBoxedHandleManager, boxed);
}
VkImage try_unbox_VkImage(BoxedHandleManager* pBoxedHandleManager, VkImage boxed) {
    return try_unbox_VkType<VkImage>(pBoxedHandleManager, boxed);
}
VkImage unboxed_to_boxed_non_dispatchable_VkImage(BoxedHandleManager* pBoxedHandleManager,
                                                  VkImage boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkImage>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkImage(BoxedHandleManager* pBoxedHandleManager, VkImage boxed,
                                        VkImage underlying) {
    set_boxed_non_dispatchable_VkType<VkImage>(pBoxedHandleManager, boxed, new_unboxed);
}
VkImageView new_boxed_non_dispatchable_VkImageView(BoxedHandleManager* pBoxedHandleManager,
                                                   VkImageView underlying) {
    return new_boxed_VkType<VkImageView>(pBoxedHandleManager, unboxed);
}
void delete_VkImageView(BoxedHandleManager* pBoxedHandleManager, VkImageView boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkImageView(BoxedHandleManager* pBoxedHandleManager, VkImageView boxed,
                                VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkImageView unbox_VkImageView(BoxedHandleManager* pBoxedHandleManager, VkImageView boxed) {
    return try_unbox_VkType<VkImageView>(pBoxedHandleManager, boxed);
}
VkImageView try_unbox_VkImageView(BoxedHandleManager* pBoxedHandleManager, VkImageView boxed) {
    return try_unbox_VkType<VkImageView>(pBoxedHandleManager, boxed);
}
VkImageView unboxed_to_boxed_non_dispatchable_VkImageView(BoxedHandleManager* pBoxedHandleManager,
                                                          VkImageView boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkImageView>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkImageView(BoxedHandleManager* pBoxedHandleManager,
                                            VkImageView boxed, VkImageView underlying) {
    set_boxed_non_dispatchable_VkType<VkImageView>(pBoxedHandleManager, boxed, new_unboxed);
}
VkShaderModule new_boxed_non_dispatchable_VkShaderModule(BoxedHandleManager* pBoxedHandleManager,
                                                         VkShaderModule underlying) {
    return new_boxed_VkType<VkShaderModule>(pBoxedHandleManager, unboxed);
}
void delete_VkShaderModule(BoxedHandleManager* pBoxedHandleManager, VkShaderModule boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkShaderModule(BoxedHandleManager* pBoxedHandleManager, VkShaderModule boxed,
                                   VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkShaderModule unbox_VkShaderModule(BoxedHandleManager* pBoxedHandleManager, VkShaderModule boxed) {
    return try_unbox_VkType<VkShaderModule>(pBoxedHandleManager, boxed);
}
VkShaderModule try_unbox_VkShaderModule(BoxedHandleManager* pBoxedHandleManager,
                                        VkShaderModule boxed) {
    return try_unbox_VkType<VkShaderModule>(pBoxedHandleManager, boxed);
}
VkShaderModule unboxed_to_boxed_non_dispatchable_VkShaderModule(
    BoxedHandleManager* pBoxedHandleManager, VkShaderModule boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkShaderModule>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkShaderModule(BoxedHandleManager* pBoxedHandleManager,
                                               VkShaderModule boxed, VkShaderModule underlying) {
    set_boxed_non_dispatchable_VkType<VkShaderModule>(pBoxedHandleManager, boxed, new_unboxed);
}
VkDescriptorPool new_boxed_non_dispatchable_VkDescriptorPool(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorPool underlying) {
    return new_boxed_VkType<VkDescriptorPool>(pBoxedHandleManager, unboxed);
}
void delete_VkDescriptorPool(BoxedHandleManager* pBoxedHandleManager, VkDescriptorPool boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDescriptorPool(BoxedHandleManager* pBoxedHandleManager,
                                     VkDescriptorPool boxed, VkDevice device,
                                     std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDescriptorPool unbox_VkDescriptorPool(BoxedHandleManager* pBoxedHandleManager,
                                        VkDescriptorPool boxed) {
    return try_unbox_VkType<VkDescriptorPool>(pBoxedHandleManager, boxed);
}
VkDescriptorPool try_unbox_VkDescriptorPool(BoxedHandleManager* pBoxedHandleManager,
                                            VkDescriptorPool boxed) {
    return try_unbox_VkType<VkDescriptorPool>(pBoxedHandleManager, boxed);
}
VkDescriptorPool unboxed_to_boxed_non_dispatchable_VkDescriptorPool(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorPool boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorPool>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkDescriptorPool(BoxedHandleManager* pBoxedHandleManager,
                                                 VkDescriptorPool boxed,
                                                 VkDescriptorPool underlying) {
    set_boxed_non_dispatchable_VkType<VkDescriptorPool>(pBoxedHandleManager, boxed, new_unboxed);
}
VkDescriptorSetLayout new_boxed_non_dispatchable_VkDescriptorSetLayout(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorSetLayout underlying) {
    return new_boxed_VkType<VkDescriptorSetLayout>(pBoxedHandleManager, unboxed);
}
void delete_VkDescriptorSetLayout(BoxedHandleManager* pBoxedHandleManager,
                                  VkDescriptorSetLayout boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDescriptorSetLayout(BoxedHandleManager* pBoxedHandleManager,
                                          VkDescriptorSetLayout boxed, VkDevice device,
                                          std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDescriptorSetLayout unbox_VkDescriptorSetLayout(BoxedHandleManager* pBoxedHandleManager,
                                                  VkDescriptorSetLayout boxed) {
    return try_unbox_VkType<VkDescriptorSetLayout>(pBoxedHandleManager, boxed);
}
VkDescriptorSetLayout try_unbox_VkDescriptorSetLayout(BoxedHandleManager* pBoxedHandleManager,
                                                      VkDescriptorSetLayout boxed) {
    return try_unbox_VkType<VkDescriptorSetLayout>(pBoxedHandleManager, boxed);
}
VkDescriptorSetLayout unboxed_to_boxed_non_dispatchable_VkDescriptorSetLayout(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorSetLayout boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorSetLayout>(pBoxedHandleManager,
                                                                           unboxed);
}
void set_boxed_non_dispatchable_VkDescriptorSetLayout(BoxedHandleManager* pBoxedHandleManager,
                                                      VkDescriptorSetLayout boxed,
                                                      VkDescriptorSetLayout underlying) {
    set_boxed_non_dispatchable_VkType<VkDescriptorSetLayout>(pBoxedHandleManager, boxed,
                                                             new_unboxed);
}
VkDescriptorSet new_boxed_non_dispatchable_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager,
                                                           VkDescriptorSet underlying) {
    return new_boxed_VkType<VkDescriptorSet>(pBoxedHandleManager, unboxed);
}
void delete_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager, VkDescriptorSet boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager, VkDescriptorSet boxed,
                                    VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDescriptorSet unbox_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager,
                                      VkDescriptorSet boxed) {
    return try_unbox_VkType<VkDescriptorSet>(pBoxedHandleManager, boxed);
}
VkDescriptorSet try_unbox_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager,
                                          VkDescriptorSet boxed) {
    return try_unbox_VkType<VkDescriptorSet>(pBoxedHandleManager, boxed);
}
VkDescriptorSet unboxed_to_boxed_non_dispatchable_VkDescriptorSet(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorSet boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorSet>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkDescriptorSet(BoxedHandleManager* pBoxedHandleManager,
                                                VkDescriptorSet boxed, VkDescriptorSet underlying) {
    set_boxed_non_dispatchable_VkType<VkDescriptorSet>(pBoxedHandleManager, boxed, new_unboxed);
}
VkSampler new_boxed_non_dispatchable_VkSampler(BoxedHandleManager* pBoxedHandleManager,
                                               VkSampler underlying) {
    return new_boxed_VkType<VkSampler>(pBoxedHandleManager, unboxed);
}
void delete_VkSampler(BoxedHandleManager* pBoxedHandleManager, VkSampler boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkSampler(BoxedHandleManager* pBoxedHandleManager, VkSampler boxed,
                              VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkSampler unbox_VkSampler(BoxedHandleManager* pBoxedHandleManager, VkSampler boxed) {
    return try_unbox_VkType<VkSampler>(pBoxedHandleManager, boxed);
}
VkSampler try_unbox_VkSampler(BoxedHandleManager* pBoxedHandleManager, VkSampler boxed) {
    return try_unbox_VkType<VkSampler>(pBoxedHandleManager, boxed);
}
VkSampler unboxed_to_boxed_non_dispatchable_VkSampler(BoxedHandleManager* pBoxedHandleManager,
                                                      VkSampler boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSampler>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkSampler(BoxedHandleManager* pBoxedHandleManager, VkSampler boxed,
                                          VkSampler underlying) {
    set_boxed_non_dispatchable_VkType<VkSampler>(pBoxedHandleManager, boxed, new_unboxed);
}
VkPipeline new_boxed_non_dispatchable_VkPipeline(BoxedHandleManager* pBoxedHandleManager,
                                                 VkPipeline underlying) {
    return new_boxed_VkType<VkPipeline>(pBoxedHandleManager, unboxed);
}
void delete_VkPipeline(BoxedHandleManager* pBoxedHandleManager, VkPipeline boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkPipeline(BoxedHandleManager* pBoxedHandleManager, VkPipeline boxed,
                               VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkPipeline unbox_VkPipeline(BoxedHandleManager* pBoxedHandleManager, VkPipeline boxed) {
    return try_unbox_VkType<VkPipeline>(pBoxedHandleManager, boxed);
}
VkPipeline try_unbox_VkPipeline(BoxedHandleManager* pBoxedHandleManager, VkPipeline boxed) {
    return try_unbox_VkType<VkPipeline>(pBoxedHandleManager, boxed);
}
VkPipeline unboxed_to_boxed_non_dispatchable_VkPipeline(BoxedHandleManager* pBoxedHandleManager,
                                                        VkPipeline boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipeline>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkPipeline(BoxedHandleManager* pBoxedHandleManager,
                                           VkPipeline boxed, VkPipeline underlying) {
    set_boxed_non_dispatchable_VkType<VkPipeline>(pBoxedHandleManager, boxed, new_unboxed);
}
VkPipelineCache new_boxed_non_dispatchable_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager,
                                                           VkPipelineCache underlying) {
    return new_boxed_VkType<VkPipelineCache>(pBoxedHandleManager, unboxed);
}
void delete_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager, VkPipelineCache boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager, VkPipelineCache boxed,
                                    VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkPipelineCache unbox_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager,
                                      VkPipelineCache boxed) {
    return try_unbox_VkType<VkPipelineCache>(pBoxedHandleManager, boxed);
}
VkPipelineCache try_unbox_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager,
                                          VkPipelineCache boxed) {
    return try_unbox_VkType<VkPipelineCache>(pBoxedHandleManager, boxed);
}
VkPipelineCache unboxed_to_boxed_non_dispatchable_VkPipelineCache(
    BoxedHandleManager* pBoxedHandleManager, VkPipelineCache boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipelineCache>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkPipelineCache(BoxedHandleManager* pBoxedHandleManager,
                                                VkPipelineCache boxed, VkPipelineCache underlying) {
    set_boxed_non_dispatchable_VkType<VkPipelineCache>(pBoxedHandleManager, boxed, new_unboxed);
}
VkPipelineLayout new_boxed_non_dispatchable_VkPipelineLayout(
    BoxedHandleManager* pBoxedHandleManager, VkPipelineLayout underlying) {
    return new_boxed_VkType<VkPipelineLayout>(pBoxedHandleManager, unboxed);
}
void delete_VkPipelineLayout(BoxedHandleManager* pBoxedHandleManager, VkPipelineLayout boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkPipelineLayout(BoxedHandleManager* pBoxedHandleManager,
                                     VkPipelineLayout boxed, VkDevice device,
                                     std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkPipelineLayout unbox_VkPipelineLayout(BoxedHandleManager* pBoxedHandleManager,
                                        VkPipelineLayout boxed) {
    return try_unbox_VkType<VkPipelineLayout>(pBoxedHandleManager, boxed);
}
VkPipelineLayout try_unbox_VkPipelineLayout(BoxedHandleManager* pBoxedHandleManager,
                                            VkPipelineLayout boxed) {
    return try_unbox_VkType<VkPipelineLayout>(pBoxedHandleManager, boxed);
}
VkPipelineLayout unboxed_to_boxed_non_dispatchable_VkPipelineLayout(
    BoxedHandleManager* pBoxedHandleManager, VkPipelineLayout boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPipelineLayout>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkPipelineLayout(BoxedHandleManager* pBoxedHandleManager,
                                                 VkPipelineLayout boxed,
                                                 VkPipelineLayout underlying) {
    set_boxed_non_dispatchable_VkType<VkPipelineLayout>(pBoxedHandleManager, boxed, new_unboxed);
}
VkRenderPass new_boxed_non_dispatchable_VkRenderPass(BoxedHandleManager* pBoxedHandleManager,
                                                     VkRenderPass underlying) {
    return new_boxed_VkType<VkRenderPass>(pBoxedHandleManager, unboxed);
}
void delete_VkRenderPass(BoxedHandleManager* pBoxedHandleManager, VkRenderPass boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkRenderPass(BoxedHandleManager* pBoxedHandleManager, VkRenderPass boxed,
                                 VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkRenderPass unbox_VkRenderPass(BoxedHandleManager* pBoxedHandleManager, VkRenderPass boxed) {
    return try_unbox_VkType<VkRenderPass>(pBoxedHandleManager, boxed);
}
VkRenderPass try_unbox_VkRenderPass(BoxedHandleManager* pBoxedHandleManager, VkRenderPass boxed) {
    return try_unbox_VkType<VkRenderPass>(pBoxedHandleManager, boxed);
}
VkRenderPass unboxed_to_boxed_non_dispatchable_VkRenderPass(BoxedHandleManager* pBoxedHandleManager,
                                                            VkRenderPass boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkRenderPass>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkRenderPass(BoxedHandleManager* pBoxedHandleManager,
                                             VkRenderPass boxed, VkRenderPass underlying) {
    set_boxed_non_dispatchable_VkType<VkRenderPass>(pBoxedHandleManager, boxed, new_unboxed);
}
VkFramebuffer new_boxed_non_dispatchable_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager,
                                                       VkFramebuffer underlying) {
    return new_boxed_VkType<VkFramebuffer>(pBoxedHandleManager, unboxed);
}
void delete_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager, VkFramebuffer boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager, VkFramebuffer boxed,
                                  VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkFramebuffer unbox_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager, VkFramebuffer boxed) {
    return try_unbox_VkType<VkFramebuffer>(pBoxedHandleManager, boxed);
}
VkFramebuffer try_unbox_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager,
                                      VkFramebuffer boxed) {
    return try_unbox_VkType<VkFramebuffer>(pBoxedHandleManager, boxed);
}
VkFramebuffer unboxed_to_boxed_non_dispatchable_VkFramebuffer(
    BoxedHandleManager* pBoxedHandleManager, VkFramebuffer boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkFramebuffer>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkFramebuffer(BoxedHandleManager* pBoxedHandleManager,
                                              VkFramebuffer boxed, VkFramebuffer underlying) {
    set_boxed_non_dispatchable_VkType<VkFramebuffer>(pBoxedHandleManager, boxed, new_unboxed);
}
VkCommandPool new_boxed_non_dispatchable_VkCommandPool(BoxedHandleManager* pBoxedHandleManager,
                                                       VkCommandPool underlying) {
    return new_boxed_VkType<VkCommandPool>(pBoxedHandleManager, unboxed);
}
void delete_VkCommandPool(BoxedHandleManager* pBoxedHandleManager, VkCommandPool boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkCommandPool(BoxedHandleManager* pBoxedHandleManager, VkCommandPool boxed,
                                  VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkCommandPool unbox_VkCommandPool(BoxedHandleManager* pBoxedHandleManager, VkCommandPool boxed) {
    return try_unbox_VkType<VkCommandPool>(pBoxedHandleManager, boxed);
}
VkCommandPool try_unbox_VkCommandPool(BoxedHandleManager* pBoxedHandleManager,
                                      VkCommandPool boxed) {
    return try_unbox_VkType<VkCommandPool>(pBoxedHandleManager, boxed);
}
VkCommandPool unboxed_to_boxed_non_dispatchable_VkCommandPool(
    BoxedHandleManager* pBoxedHandleManager, VkCommandPool boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCommandPool>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkCommandPool(BoxedHandleManager* pBoxedHandleManager,
                                              VkCommandPool boxed, VkCommandPool underlying) {
    set_boxed_non_dispatchable_VkType<VkCommandPool>(pBoxedHandleManager, boxed, new_unboxed);
}
VkFence new_boxed_non_dispatchable_VkFence(BoxedHandleManager* pBoxedHandleManager,
                                           VkFence underlying) {
    return new_boxed_VkType<VkFence>(pBoxedHandleManager, unboxed);
}
void delete_VkFence(BoxedHandleManager* pBoxedHandleManager, VkFence boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkFence(BoxedHandleManager* pBoxedHandleManager, VkFence boxed, VkDevice device,
                            std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkFence unbox_VkFence(BoxedHandleManager* pBoxedHandleManager, VkFence boxed) {
    return try_unbox_VkType<VkFence>(pBoxedHandleManager, boxed);
}
VkFence try_unbox_VkFence(BoxedHandleManager* pBoxedHandleManager, VkFence boxed) {
    return try_unbox_VkType<VkFence>(pBoxedHandleManager, boxed);
}
VkFence unboxed_to_boxed_non_dispatchable_VkFence(BoxedHandleManager* pBoxedHandleManager,
                                                  VkFence boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkFence>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkFence(BoxedHandleManager* pBoxedHandleManager, VkFence boxed,
                                        VkFence underlying) {
    set_boxed_non_dispatchable_VkType<VkFence>(pBoxedHandleManager, boxed, new_unboxed);
}
VkSemaphore new_boxed_non_dispatchable_VkSemaphore(BoxedHandleManager* pBoxedHandleManager,
                                                   VkSemaphore underlying) {
    return new_boxed_VkType<VkSemaphore>(pBoxedHandleManager, unboxed);
}
void delete_VkSemaphore(BoxedHandleManager* pBoxedHandleManager, VkSemaphore boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkSemaphore(BoxedHandleManager* pBoxedHandleManager, VkSemaphore boxed,
                                VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkSemaphore unbox_VkSemaphore(BoxedHandleManager* pBoxedHandleManager, VkSemaphore boxed) {
    return try_unbox_VkType<VkSemaphore>(pBoxedHandleManager, boxed);
}
VkSemaphore try_unbox_VkSemaphore(BoxedHandleManager* pBoxedHandleManager, VkSemaphore boxed) {
    return try_unbox_VkType<VkSemaphore>(pBoxedHandleManager, boxed);
}
VkSemaphore unboxed_to_boxed_non_dispatchable_VkSemaphore(BoxedHandleManager* pBoxedHandleManager,
                                                          VkSemaphore boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSemaphore>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkSemaphore(BoxedHandleManager* pBoxedHandleManager,
                                            VkSemaphore boxed, VkSemaphore underlying) {
    set_boxed_non_dispatchable_VkType<VkSemaphore>(pBoxedHandleManager, boxed, new_unboxed);
}
VkEvent new_boxed_non_dispatchable_VkEvent(BoxedHandleManager* pBoxedHandleManager,
                                           VkEvent underlying) {
    return new_boxed_VkType<VkEvent>(pBoxedHandleManager, unboxed);
}
void delete_VkEvent(BoxedHandleManager* pBoxedHandleManager, VkEvent boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkEvent(BoxedHandleManager* pBoxedHandleManager, VkEvent boxed, VkDevice device,
                            std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkEvent unbox_VkEvent(BoxedHandleManager* pBoxedHandleManager, VkEvent boxed) {
    return try_unbox_VkType<VkEvent>(pBoxedHandleManager, boxed);
}
VkEvent try_unbox_VkEvent(BoxedHandleManager* pBoxedHandleManager, VkEvent boxed) {
    return try_unbox_VkType<VkEvent>(pBoxedHandleManager, boxed);
}
VkEvent unboxed_to_boxed_non_dispatchable_VkEvent(BoxedHandleManager* pBoxedHandleManager,
                                                  VkEvent boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkEvent>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkEvent(BoxedHandleManager* pBoxedHandleManager, VkEvent boxed,
                                        VkEvent underlying) {
    set_boxed_non_dispatchable_VkType<VkEvent>(pBoxedHandleManager, boxed, new_unboxed);
}
VkQueryPool new_boxed_non_dispatchable_VkQueryPool(BoxedHandleManager* pBoxedHandleManager,
                                                   VkQueryPool underlying) {
    return new_boxed_VkType<VkQueryPool>(pBoxedHandleManager, unboxed);
}
void delete_VkQueryPool(BoxedHandleManager* pBoxedHandleManager, VkQueryPool boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkQueryPool(BoxedHandleManager* pBoxedHandleManager, VkQueryPool boxed,
                                VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkQueryPool unbox_VkQueryPool(BoxedHandleManager* pBoxedHandleManager, VkQueryPool boxed) {
    return try_unbox_VkType<VkQueryPool>(pBoxedHandleManager, boxed);
}
VkQueryPool try_unbox_VkQueryPool(BoxedHandleManager* pBoxedHandleManager, VkQueryPool boxed) {
    return try_unbox_VkType<VkQueryPool>(pBoxedHandleManager, boxed);
}
VkQueryPool unboxed_to_boxed_non_dispatchable_VkQueryPool(BoxedHandleManager* pBoxedHandleManager,
                                                          VkQueryPool boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkQueryPool>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkQueryPool(BoxedHandleManager* pBoxedHandleManager,
                                            VkQueryPool boxed, VkQueryPool underlying) {
    set_boxed_non_dispatchable_VkType<VkQueryPool>(pBoxedHandleManager, boxed, new_unboxed);
}
VkSamplerYcbcrConversion new_boxed_non_dispatchable_VkSamplerYcbcrConversion(
    BoxedHandleManager* pBoxedHandleManager, VkSamplerYcbcrConversion underlying) {
    return new_boxed_VkType<VkSamplerYcbcrConversion>(pBoxedHandleManager, unboxed);
}
void delete_VkSamplerYcbcrConversion(BoxedHandleManager* pBoxedHandleManager,
                                     VkSamplerYcbcrConversion boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkSamplerYcbcrConversion(BoxedHandleManager* pBoxedHandleManager,
                                             VkSamplerYcbcrConversion boxed, VkDevice device,
                                             std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkSamplerYcbcrConversion unbox_VkSamplerYcbcrConversion(BoxedHandleManager* pBoxedHandleManager,
                                                        VkSamplerYcbcrConversion boxed) {
    return try_unbox_VkType<VkSamplerYcbcrConversion>(pBoxedHandleManager, boxed);
}
VkSamplerYcbcrConversion try_unbox_VkSamplerYcbcrConversion(BoxedHandleManager* pBoxedHandleManager,
                                                            VkSamplerYcbcrConversion boxed) {
    return try_unbox_VkType<VkSamplerYcbcrConversion>(pBoxedHandleManager, boxed);
}
VkSamplerYcbcrConversion unboxed_to_boxed_non_dispatchable_VkSamplerYcbcrConversion(
    BoxedHandleManager* pBoxedHandleManager, VkSamplerYcbcrConversion boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSamplerYcbcrConversion>(pBoxedHandleManager,
                                                                              unboxed);
}
void set_boxed_non_dispatchable_VkSamplerYcbcrConversion(BoxedHandleManager* pBoxedHandleManager,
                                                         VkSamplerYcbcrConversion boxed,
                                                         VkSamplerYcbcrConversion underlying) {
    set_boxed_non_dispatchable_VkType<VkSamplerYcbcrConversion>(pBoxedHandleManager, boxed,
                                                                new_unboxed);
}
VkDescriptorUpdateTemplate new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorUpdateTemplate underlying) {
    return new_boxed_VkType<VkDescriptorUpdateTemplate>(pBoxedHandleManager, unboxed);
}
void delete_VkDescriptorUpdateTemplate(BoxedHandleManager* pBoxedHandleManager,
                                       VkDescriptorUpdateTemplate boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDescriptorUpdateTemplate(BoxedHandleManager* pBoxedHandleManager,
                                               VkDescriptorUpdateTemplate boxed, VkDevice device,
                                               std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDescriptorUpdateTemplate unbox_VkDescriptorUpdateTemplate(BoxedHandleManager* pBoxedHandleManager,
                                                            VkDescriptorUpdateTemplate boxed) {
    return try_unbox_VkType<VkDescriptorUpdateTemplate>(pBoxedHandleManager, boxed);
}
VkDescriptorUpdateTemplate try_unbox_VkDescriptorUpdateTemplate(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorUpdateTemplate boxed) {
    return try_unbox_VkType<VkDescriptorUpdateTemplate>(pBoxedHandleManager, boxed);
}
VkDescriptorUpdateTemplate unboxed_to_boxed_non_dispatchable_VkDescriptorUpdateTemplate(
    BoxedHandleManager* pBoxedHandleManager, VkDescriptorUpdateTemplate boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDescriptorUpdateTemplate>(pBoxedHandleManager,
                                                                                unboxed);
}
void set_boxed_non_dispatchable_VkDescriptorUpdateTemplate(BoxedHandleManager* pBoxedHandleManager,
                                                           VkDescriptorUpdateTemplate boxed,
                                                           VkDescriptorUpdateTemplate underlying) {
    set_boxed_non_dispatchable_VkType<VkDescriptorUpdateTemplate>(pBoxedHandleManager, boxed,
                                                                  new_unboxed);
}
VkSurfaceKHR new_boxed_non_dispatchable_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager,
                                                     VkSurfaceKHR underlying) {
    return new_boxed_VkType<VkSurfaceKHR>(pBoxedHandleManager, unboxed);
}
void delete_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager, VkSurfaceKHR boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager, VkSurfaceKHR boxed,
                                 VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkSurfaceKHR unbox_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager, VkSurfaceKHR boxed) {
    return try_unbox_VkType<VkSurfaceKHR>(pBoxedHandleManager, boxed);
}
VkSurfaceKHR try_unbox_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager, VkSurfaceKHR boxed) {
    return try_unbox_VkType<VkSurfaceKHR>(pBoxedHandleManager, boxed);
}
VkSurfaceKHR unboxed_to_boxed_non_dispatchable_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager,
                                                            VkSurfaceKHR boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSurfaceKHR>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkSurfaceKHR(BoxedHandleManager* pBoxedHandleManager,
                                             VkSurfaceKHR boxed, VkSurfaceKHR underlying) {
    set_boxed_non_dispatchable_VkType<VkSurfaceKHR>(pBoxedHandleManager, boxed, new_unboxed);
}
VkSwapchainKHR new_boxed_non_dispatchable_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager,
                                                         VkSwapchainKHR underlying) {
    return new_boxed_VkType<VkSwapchainKHR>(pBoxedHandleManager, unboxed);
}
void delete_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager, VkSwapchainKHR boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager, VkSwapchainKHR boxed,
                                   VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkSwapchainKHR unbox_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager, VkSwapchainKHR boxed) {
    return try_unbox_VkType<VkSwapchainKHR>(pBoxedHandleManager, boxed);
}
VkSwapchainKHR try_unbox_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager,
                                        VkSwapchainKHR boxed) {
    return try_unbox_VkType<VkSwapchainKHR>(pBoxedHandleManager, boxed);
}
VkSwapchainKHR unboxed_to_boxed_non_dispatchable_VkSwapchainKHR(
    BoxedHandleManager* pBoxedHandleManager, VkSwapchainKHR boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkSwapchainKHR>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkSwapchainKHR(BoxedHandleManager* pBoxedHandleManager,
                                               VkSwapchainKHR boxed, VkSwapchainKHR underlying) {
    set_boxed_non_dispatchable_VkType<VkSwapchainKHR>(pBoxedHandleManager, boxed, new_unboxed);
}
VkDisplayKHR new_boxed_non_dispatchable_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager,
                                                     VkDisplayKHR underlying) {
    return new_boxed_VkType<VkDisplayKHR>(pBoxedHandleManager, unboxed);
}
void delete_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager, VkDisplayKHR boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager, VkDisplayKHR boxed,
                                 VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDisplayKHR unbox_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager, VkDisplayKHR boxed) {
    return try_unbox_VkType<VkDisplayKHR>(pBoxedHandleManager, boxed);
}
VkDisplayKHR try_unbox_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager, VkDisplayKHR boxed) {
    return try_unbox_VkType<VkDisplayKHR>(pBoxedHandleManager, boxed);
}
VkDisplayKHR unboxed_to_boxed_non_dispatchable_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager,
                                                            VkDisplayKHR boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDisplayKHR>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkDisplayKHR(BoxedHandleManager* pBoxedHandleManager,
                                             VkDisplayKHR boxed, VkDisplayKHR underlying) {
    set_boxed_non_dispatchable_VkType<VkDisplayKHR>(pBoxedHandleManager, boxed, new_unboxed);
}
VkDisplayModeKHR new_boxed_non_dispatchable_VkDisplayModeKHR(
    BoxedHandleManager* pBoxedHandleManager, VkDisplayModeKHR underlying) {
    return new_boxed_VkType<VkDisplayModeKHR>(pBoxedHandleManager, unboxed);
}
void delete_VkDisplayModeKHR(BoxedHandleManager* pBoxedHandleManager, VkDisplayModeKHR boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDisplayModeKHR(BoxedHandleManager* pBoxedHandleManager,
                                     VkDisplayModeKHR boxed, VkDevice device,
                                     std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDisplayModeKHR unbox_VkDisplayModeKHR(BoxedHandleManager* pBoxedHandleManager,
                                        VkDisplayModeKHR boxed) {
    return try_unbox_VkType<VkDisplayModeKHR>(pBoxedHandleManager, boxed);
}
VkDisplayModeKHR try_unbox_VkDisplayModeKHR(BoxedHandleManager* pBoxedHandleManager,
                                            VkDisplayModeKHR boxed) {
    return try_unbox_VkType<VkDisplayModeKHR>(pBoxedHandleManager, boxed);
}
VkDisplayModeKHR unboxed_to_boxed_non_dispatchable_VkDisplayModeKHR(
    BoxedHandleManager* pBoxedHandleManager, VkDisplayModeKHR boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDisplayModeKHR>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkDisplayModeKHR(BoxedHandleManager* pBoxedHandleManager,
                                                 VkDisplayModeKHR boxed,
                                                 VkDisplayModeKHR underlying) {
    set_boxed_non_dispatchable_VkType<VkDisplayModeKHR>(pBoxedHandleManager, boxed, new_unboxed);
}
VkValidationCacheEXT new_boxed_non_dispatchable_VkValidationCacheEXT(
    BoxedHandleManager* pBoxedHandleManager, VkValidationCacheEXT underlying) {
    return new_boxed_VkType<VkValidationCacheEXT>(pBoxedHandleManager, unboxed);
}
void delete_VkValidationCacheEXT(BoxedHandleManager* pBoxedHandleManager,
                                 VkValidationCacheEXT boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkValidationCacheEXT(BoxedHandleManager* pBoxedHandleManager,
                                         VkValidationCacheEXT boxed, VkDevice device,
                                         std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkValidationCacheEXT unbox_VkValidationCacheEXT(BoxedHandleManager* pBoxedHandleManager,
                                                VkValidationCacheEXT boxed) {
    return try_unbox_VkType<VkValidationCacheEXT>(pBoxedHandleManager, boxed);
}
VkValidationCacheEXT try_unbox_VkValidationCacheEXT(BoxedHandleManager* pBoxedHandleManager,
                                                    VkValidationCacheEXT boxed) {
    return try_unbox_VkType<VkValidationCacheEXT>(pBoxedHandleManager, boxed);
}
VkValidationCacheEXT unboxed_to_boxed_non_dispatchable_VkValidationCacheEXT(
    BoxedHandleManager* pBoxedHandleManager, VkValidationCacheEXT boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkValidationCacheEXT>(pBoxedHandleManager,
                                                                          unboxed);
}
void set_boxed_non_dispatchable_VkValidationCacheEXT(BoxedHandleManager* pBoxedHandleManager,
                                                     VkValidationCacheEXT boxed,
                                                     VkValidationCacheEXT underlying) {
    set_boxed_non_dispatchable_VkType<VkValidationCacheEXT>(pBoxedHandleManager, boxed,
                                                            new_unboxed);
}
VkDebugReportCallbackEXT new_boxed_non_dispatchable_VkDebugReportCallbackEXT(
    BoxedHandleManager* pBoxedHandleManager, VkDebugReportCallbackEXT underlying) {
    return new_boxed_VkType<VkDebugReportCallbackEXT>(pBoxedHandleManager, unboxed);
}
void delete_VkDebugReportCallbackEXT(BoxedHandleManager* pBoxedHandleManager,
                                     VkDebugReportCallbackEXT boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDebugReportCallbackEXT(BoxedHandleManager* pBoxedHandleManager,
                                             VkDebugReportCallbackEXT boxed, VkDevice device,
                                             std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDebugReportCallbackEXT unbox_VkDebugReportCallbackEXT(BoxedHandleManager* pBoxedHandleManager,
                                                        VkDebugReportCallbackEXT boxed) {
    return try_unbox_VkType<VkDebugReportCallbackEXT>(pBoxedHandleManager, boxed);
}
VkDebugReportCallbackEXT try_unbox_VkDebugReportCallbackEXT(BoxedHandleManager* pBoxedHandleManager,
                                                            VkDebugReportCallbackEXT boxed) {
    return try_unbox_VkType<VkDebugReportCallbackEXT>(pBoxedHandleManager, boxed);
}
VkDebugReportCallbackEXT unboxed_to_boxed_non_dispatchable_VkDebugReportCallbackEXT(
    BoxedHandleManager* pBoxedHandleManager, VkDebugReportCallbackEXT boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDebugReportCallbackEXT>(pBoxedHandleManager,
                                                                              unboxed);
}
void set_boxed_non_dispatchable_VkDebugReportCallbackEXT(BoxedHandleManager* pBoxedHandleManager,
                                                         VkDebugReportCallbackEXT boxed,
                                                         VkDebugReportCallbackEXT underlying) {
    set_boxed_non_dispatchable_VkType<VkDebugReportCallbackEXT>(pBoxedHandleManager, boxed,
                                                                new_unboxed);
}
VkDebugUtilsMessengerEXT new_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(
    BoxedHandleManager* pBoxedHandleManager, VkDebugUtilsMessengerEXT underlying) {
    return new_boxed_VkType<VkDebugUtilsMessengerEXT>(pBoxedHandleManager, unboxed);
}
void delete_VkDebugUtilsMessengerEXT(BoxedHandleManager* pBoxedHandleManager,
                                     VkDebugUtilsMessengerEXT boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkDebugUtilsMessengerEXT(BoxedHandleManager* pBoxedHandleManager,
                                             VkDebugUtilsMessengerEXT boxed, VkDevice device,
                                             std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkDebugUtilsMessengerEXT unbox_VkDebugUtilsMessengerEXT(BoxedHandleManager* pBoxedHandleManager,
                                                        VkDebugUtilsMessengerEXT boxed) {
    return try_unbox_VkType<VkDebugUtilsMessengerEXT>(pBoxedHandleManager, boxed);
}
VkDebugUtilsMessengerEXT try_unbox_VkDebugUtilsMessengerEXT(BoxedHandleManager* pBoxedHandleManager,
                                                            VkDebugUtilsMessengerEXT boxed) {
    return try_unbox_VkType<VkDebugUtilsMessengerEXT>(pBoxedHandleManager, boxed);
}
VkDebugUtilsMessengerEXT unboxed_to_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(
    BoxedHandleManager* pBoxedHandleManager, VkDebugUtilsMessengerEXT boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkDebugUtilsMessengerEXT>(pBoxedHandleManager,
                                                                              unboxed);
}
void set_boxed_non_dispatchable_VkDebugUtilsMessengerEXT(BoxedHandleManager* pBoxedHandleManager,
                                                         VkDebugUtilsMessengerEXT boxed,
                                                         VkDebugUtilsMessengerEXT underlying) {
    set_boxed_non_dispatchable_VkType<VkDebugUtilsMessengerEXT>(pBoxedHandleManager, boxed,
                                                                new_unboxed);
}
VkAccelerationStructureNV new_boxed_non_dispatchable_VkAccelerationStructureNV(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureNV underlying) {
    return new_boxed_VkType<VkAccelerationStructureNV>(pBoxedHandleManager, unboxed);
}
void delete_VkAccelerationStructureNV(BoxedHandleManager* pBoxedHandleManager,
                                      VkAccelerationStructureNV boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkAccelerationStructureNV(BoxedHandleManager* pBoxedHandleManager,
                                              VkAccelerationStructureNV boxed, VkDevice device,
                                              std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkAccelerationStructureNV unbox_VkAccelerationStructureNV(BoxedHandleManager* pBoxedHandleManager,
                                                          VkAccelerationStructureNV boxed) {
    return try_unbox_VkType<VkAccelerationStructureNV>(pBoxedHandleManager, boxed);
}
VkAccelerationStructureNV try_unbox_VkAccelerationStructureNV(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureNV boxed) {
    return try_unbox_VkType<VkAccelerationStructureNV>(pBoxedHandleManager, boxed);
}
VkAccelerationStructureNV unboxed_to_boxed_non_dispatchable_VkAccelerationStructureNV(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureNV boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkAccelerationStructureNV>(pBoxedHandleManager,
                                                                               unboxed);
}
void set_boxed_non_dispatchable_VkAccelerationStructureNV(BoxedHandleManager* pBoxedHandleManager,
                                                          VkAccelerationStructureNV boxed,
                                                          VkAccelerationStructureNV underlying) {
    set_boxed_non_dispatchable_VkType<VkAccelerationStructureNV>(pBoxedHandleManager, boxed,
                                                                 new_unboxed);
}
VkIndirectCommandsLayoutNV new_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(
    BoxedHandleManager* pBoxedHandleManager, VkIndirectCommandsLayoutNV underlying) {
    return new_boxed_VkType<VkIndirectCommandsLayoutNV>(pBoxedHandleManager, unboxed);
}
void delete_VkIndirectCommandsLayoutNV(BoxedHandleManager* pBoxedHandleManager,
                                       VkIndirectCommandsLayoutNV boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkIndirectCommandsLayoutNV(BoxedHandleManager* pBoxedHandleManager,
                                               VkIndirectCommandsLayoutNV boxed, VkDevice device,
                                               std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkIndirectCommandsLayoutNV unbox_VkIndirectCommandsLayoutNV(BoxedHandleManager* pBoxedHandleManager,
                                                            VkIndirectCommandsLayoutNV boxed) {
    return try_unbox_VkType<VkIndirectCommandsLayoutNV>(pBoxedHandleManager, boxed);
}
VkIndirectCommandsLayoutNV try_unbox_VkIndirectCommandsLayoutNV(
    BoxedHandleManager* pBoxedHandleManager, VkIndirectCommandsLayoutNV boxed) {
    return try_unbox_VkType<VkIndirectCommandsLayoutNV>(pBoxedHandleManager, boxed);
}
VkIndirectCommandsLayoutNV unboxed_to_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(
    BoxedHandleManager* pBoxedHandleManager, VkIndirectCommandsLayoutNV boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkIndirectCommandsLayoutNV>(pBoxedHandleManager,
                                                                                unboxed);
}
void set_boxed_non_dispatchable_VkIndirectCommandsLayoutNV(BoxedHandleManager* pBoxedHandleManager,
                                                           VkIndirectCommandsLayoutNV boxed,
                                                           VkIndirectCommandsLayoutNV underlying) {
    set_boxed_non_dispatchable_VkType<VkIndirectCommandsLayoutNV>(pBoxedHandleManager, boxed,
                                                                  new_unboxed);
}
VkAccelerationStructureKHR new_boxed_non_dispatchable_VkAccelerationStructureKHR(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureKHR underlying) {
    return new_boxed_VkType<VkAccelerationStructureKHR>(pBoxedHandleManager, unboxed);
}
void delete_VkAccelerationStructureKHR(BoxedHandleManager* pBoxedHandleManager,
                                       VkAccelerationStructureKHR boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkAccelerationStructureKHR(BoxedHandleManager* pBoxedHandleManager,
                                               VkAccelerationStructureKHR boxed, VkDevice device,
                                               std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkAccelerationStructureKHR unbox_VkAccelerationStructureKHR(BoxedHandleManager* pBoxedHandleManager,
                                                            VkAccelerationStructureKHR boxed) {
    return try_unbox_VkType<VkAccelerationStructureKHR>(pBoxedHandleManager, boxed);
}
VkAccelerationStructureKHR try_unbox_VkAccelerationStructureKHR(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureKHR boxed) {
    return try_unbox_VkType<VkAccelerationStructureKHR>(pBoxedHandleManager, boxed);
}
VkAccelerationStructureKHR unboxed_to_boxed_non_dispatchable_VkAccelerationStructureKHR(
    BoxedHandleManager* pBoxedHandleManager, VkAccelerationStructureKHR boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkAccelerationStructureKHR>(pBoxedHandleManager,
                                                                                unboxed);
}
void set_boxed_non_dispatchable_VkAccelerationStructureKHR(BoxedHandleManager* pBoxedHandleManager,
                                                           VkAccelerationStructureKHR boxed,
                                                           VkAccelerationStructureKHR underlying) {
    set_boxed_non_dispatchable_VkType<VkAccelerationStructureKHR>(pBoxedHandleManager, boxed,
                                                                  new_unboxed);
}
VkCuModuleNVX new_boxed_non_dispatchable_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager,
                                                       VkCuModuleNVX underlying) {
    return new_boxed_VkType<VkCuModuleNVX>(pBoxedHandleManager, unboxed);
}
void delete_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager, VkCuModuleNVX boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager, VkCuModuleNVX boxed,
                                  VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkCuModuleNVX unbox_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager, VkCuModuleNVX boxed) {
    return try_unbox_VkType<VkCuModuleNVX>(pBoxedHandleManager, boxed);
}
VkCuModuleNVX try_unbox_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager,
                                      VkCuModuleNVX boxed) {
    return try_unbox_VkType<VkCuModuleNVX>(pBoxedHandleManager, boxed);
}
VkCuModuleNVX unboxed_to_boxed_non_dispatchable_VkCuModuleNVX(
    BoxedHandleManager* pBoxedHandleManager, VkCuModuleNVX boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCuModuleNVX>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkCuModuleNVX(BoxedHandleManager* pBoxedHandleManager,
                                              VkCuModuleNVX boxed, VkCuModuleNVX underlying) {
    set_boxed_non_dispatchable_VkType<VkCuModuleNVX>(pBoxedHandleManager, boxed, new_unboxed);
}
VkCuFunctionNVX new_boxed_non_dispatchable_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager,
                                                           VkCuFunctionNVX underlying) {
    return new_boxed_VkType<VkCuFunctionNVX>(pBoxedHandleManager, unboxed);
}
void delete_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager, VkCuFunctionNVX boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager, VkCuFunctionNVX boxed,
                                    VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkCuFunctionNVX unbox_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager,
                                      VkCuFunctionNVX boxed) {
    return try_unbox_VkType<VkCuFunctionNVX>(pBoxedHandleManager, boxed);
}
VkCuFunctionNVX try_unbox_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager,
                                          VkCuFunctionNVX boxed) {
    return try_unbox_VkType<VkCuFunctionNVX>(pBoxedHandleManager, boxed);
}
VkCuFunctionNVX unboxed_to_boxed_non_dispatchable_VkCuFunctionNVX(
    BoxedHandleManager* pBoxedHandleManager, VkCuFunctionNVX boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkCuFunctionNVX>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkCuFunctionNVX(BoxedHandleManager* pBoxedHandleManager,
                                                VkCuFunctionNVX boxed, VkCuFunctionNVX underlying) {
    set_boxed_non_dispatchable_VkType<VkCuFunctionNVX>(pBoxedHandleManager, boxed, new_unboxed);
}
VkPrivateDataSlot new_boxed_non_dispatchable_VkPrivateDataSlot(
    BoxedHandleManager* pBoxedHandleManager, VkPrivateDataSlot underlying) {
    return new_boxed_VkType<VkPrivateDataSlot>(pBoxedHandleManager, unboxed);
}
void delete_VkPrivateDataSlot(BoxedHandleManager* pBoxedHandleManager, VkPrivateDataSlot boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkPrivateDataSlot(BoxedHandleManager* pBoxedHandleManager,
                                      VkPrivateDataSlot boxed, VkDevice device,
                                      std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkPrivateDataSlot unbox_VkPrivateDataSlot(BoxedHandleManager* pBoxedHandleManager,
                                          VkPrivateDataSlot boxed) {
    return try_unbox_VkType<VkPrivateDataSlot>(pBoxedHandleManager, boxed);
}
VkPrivateDataSlot try_unbox_VkPrivateDataSlot(BoxedHandleManager* pBoxedHandleManager,
                                              VkPrivateDataSlot boxed) {
    return try_unbox_VkType<VkPrivateDataSlot>(pBoxedHandleManager, boxed);
}
VkPrivateDataSlot unboxed_to_boxed_non_dispatchable_VkPrivateDataSlot(
    BoxedHandleManager* pBoxedHandleManager, VkPrivateDataSlot boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkPrivateDataSlot>(pBoxedHandleManager,
                                                                       unboxed);
}
void set_boxed_non_dispatchable_VkPrivateDataSlot(BoxedHandleManager* pBoxedHandleManager,
                                                  VkPrivateDataSlot boxed,
                                                  VkPrivateDataSlot underlying) {
    set_boxed_non_dispatchable_VkType<VkPrivateDataSlot>(pBoxedHandleManager, boxed, new_unboxed);
}
VkMicromapEXT new_boxed_non_dispatchable_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager,
                                                       VkMicromapEXT underlying) {
    return new_boxed_VkType<VkMicromapEXT>(pBoxedHandleManager, unboxed);
}
void delete_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager, VkMicromapEXT boxed) {
    delete_VkType(pBoxedHandleManager, boxed);
}
void delayed_delete_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager, VkMicromapEXT boxed,
                                  VkDevice device, std::function<void()> callback) {
    delayed_delete_VkType(pBoxedHandleManager, boxed, device, std::move(callback));
}
VkMicromapEXT unbox_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager, VkMicromapEXT boxed) {
    return try_unbox_VkType<VkMicromapEXT>(pBoxedHandleManager, boxed);
}
VkMicromapEXT try_unbox_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager,
                                      VkMicromapEXT boxed) {
    return try_unbox_VkType<VkMicromapEXT>(pBoxedHandleManager, boxed);
}
VkMicromapEXT unboxed_to_boxed_non_dispatchable_VkMicromapEXT(
    BoxedHandleManager* pBoxedHandleManager, VkMicromapEXT boxed) {
    return unboxed_to_boxed_non_dispatchable_VkType<VkMicromapEXT>(pBoxedHandleManager, unboxed);
}
void set_boxed_non_dispatchable_VkMicromapEXT(BoxedHandleManager* pBoxedHandleManager,
                                              VkMicromapEXT boxed, VkMicromapEXT underlying) {
    set_boxed_non_dispatchable_VkType<VkMicromapEXT>(pBoxedHandleManager, boxed, new_unboxed);
}

#endif
}  // namespace vk
}  // namespace gfxstream
