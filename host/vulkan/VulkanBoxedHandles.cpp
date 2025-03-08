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
//
// limitations under the License.

#include "VulkanBoxedHandles.h"

#include "VkDecoderGlobalState.h"
#include "VkDecoderInternalStructs.h"

namespace gfxstream {
namespace vk {

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

BoxedHandleManager& getBoxedHandleManager() {
    return vk::VkDecoderGlobalState::get()->getBoxedHandleManager();
}

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

template <typename VkObjectT>
VkObjectT new_boxed_VkType(VkObjectT underlying, bool dispatchable = false, VulkanDispatch* dispatch = nullptr, bool ownsDispatch = false) {
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
    return (VkObjectT)getBoxedHandleManager().add(info, GetTag<VkObjectT>());
}

template <typename VkObjectT>
void delete_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return;
    }

    BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        return;
    }

    releaseOrderMaintInfo(info->ordMaintInfo);

    if (info->readStream) {
        getBoxedHandleManager().sReadStreamRegistry.push(info->readStream);
        info->readStream = nullptr;
    }

    getBoxedHandleManager().remove((uint64_t)boxed);
}

template <typename VkObjectT>
void delayed_delete_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed,
                           VkDevice device, std::function<void()> callback) {
    if (boxed == VK_NULL_HANDLE) {
        return;
    }

    pBoxedHandleManager->removeDelayed((uint64_t)boxed, device, std::move(callback));
}

// Custom unbox_* functions or GOLDFISH_VK_LIST_DISPATCHABLE_CUSTOM_UNBOX_HANDLE_TYPES
// VkQueue objects can be virtual, meaning that multiple boxed queues can map into a single
// physical queue on the host GPU. Some conversion is needed for unboxing to physical.
VkQueue unbox_VkQueueImpl(BoxedHandleManager* pBoxedHandleManager, VkQueue boxed) {
    BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
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
VkObjectT unbox_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkObjectT unboxed = VK_NULL_HANDLE;

    if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        unboxed = unbox_VkQueueImpl(pBoxedHandleManager, boxed);
    } else {
        BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
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
VkObjectT try_unbox_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed) {
    if (boxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkObjectT unboxed = VK_NULL_HANDLE;

    if constexpr (std::is_same_v<VkObjectT, VkQueue>) {
        unboxed = unbox_VkQueueImpl(pBoxedHandleManager, boxed);
    } else {
        BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
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

    return (VkObjectT)getBoxedHandleManager().getBoxedFromUnboxed((uint64_t)(uintptr_t)unboxed);
}

template <typename VkObjectT>
void set_boxed_non_dispatchable_VkType(VkObjectT boxed, VkObjectT new_unboxed) {
    BoxedHandleInfo info;
    info.underlying = (uint64_t)new_unboxed;
    getBoxedHandleManager().update((uint64_t)boxed, info, GetTag<VkObjectT>());
}

template <typename VkObjectT>
OrderMaintenanceInfo* get_order_maintenance_info_VkType(BoxedHandleManager* pBoxedHandleManager,
                                                        VkObjectT boxed) {
    BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
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
VulkanMemReadingStream* get_read_stream_VkType(BoxedHandleManager* pBoxedHandleManager,
                                               VkObjectT boxed) {
    BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        return nullptr;
    }

    if (info->readStream == nullptr) {
        info->readStream = pBoxedHandleManager->sReadStreamRegistry.pop(
            VkDecoderGlobalState::get()->getFeatures());
    }

    return info->readStream;
}

template <typename VkObjectT>
VulkanDispatch* get_dispatch_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed) {
    BoxedHandleInfo* info = pBoxedHandleManager->get((uint64_t)(uintptr_t)boxed);
    if (info == nullptr) {
        ERR("Failed to unbox %s %p", GetTypeStr<VkObjectT>(), boxed);
        return nullptr;
    }
    return info->dispatch;
}

template <typename VkObjectT>
VkObjectT new_boxed_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT underlying,
                           bool dispatchable = false, VulkanDispatch* dispatch = nullptr,
                           bool ownsDispatch = false) {
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
    auto boxed = (VkObjectT)pBoxedHandleManager->add(info, GetTag<VkObjectT>());
    auto vkboxed = (VkObjectT)boxed;
    return vkboxed;
}

template <typename VkObjectT>
VkObjectT unboxed_to_boxed_non_dispatchable_VkType(BoxedHandleManager* pBoxedHandleManager,
                                                   VkObjectT unboxed) {
    if (unboxed == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    return (VkObjectT)pBoxedHandleManager->getBoxedFromUnboxed((uint64_t)(uintptr_t)unboxed);
}

template <typename VkObjectT>
void set_boxed_non_dispatchable_VkType(BoxedHandleManager* pBoxedHandleManager, VkObjectT boxed,
                                       VkObjectT new_unboxed) {
    BoxedHandleInfo info;
    info.underlying = (uint64_t)new_unboxed;
    pBoxedHandleManager->update((uint64_t)boxed, info, GetTag<VkObjectT>());
}

// bohu-TODO
// new interface that takes ptr to BoxedHandleManager
#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEFINE(type)                                         \
    type BoxedHandleManager::new_boxed_##type(type underlying, VulkanDispatch* dispatch,          \
                                              bool ownDispatch) {                                 \
        return new_boxed_VkType<type>(this, underlying, true, dispatch, ownDispatch);             \
    }                                                                                             \
    void BoxedHandleManager::delete_##type(type boxed) {                                          \
        return delete_VkType<type>(this, boxed);                                                  \
    }                                                                                             \
    type BoxedHandleManager::unbox_##type(type boxed) { return unbox_VkType<type>(this, boxed); } \
    type BoxedHandleManager::try_unbox_##type(type boxed) {                                       \
        return try_unbox_VkType<type>(this, boxed);                                               \
    }                                                                                             \
    VulkanDispatch* BoxedHandleManager::dispatch_##type(type boxed) {                             \
        return get_dispatch_VkType<type>(this, boxed);                                            \
    }                                                                                             \
    OrderMaintenanceInfo* BoxedHandleManager::ordmaint_##type(type boxed) {                       \
        return get_order_maintenance_info_VkType<type>(this, boxed);                              \
    }                                                                                             \
    VulkanMemReadingStream* BoxedHandleManager::readstream_##type(type boxed) {                   \
        return get_read_stream_VkType<type>(this, boxed);                                         \
    }                                                                                             \
    type BoxedHandleManager::unboxed_to_boxed_##type(type unboxed) {                              \
        return unboxed_to_boxed_non_dispatchable_VkType<type>(this, unboxed);                     \
    }

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEFINE(type)                                     \
    type BoxedHandleManager::new_boxed_non_dispatchable_##type(type underlying) {                 \
        return new_boxed_VkType<type>(this, underlying);                                          \
    }                                                                                             \
    type BoxedHandleManager::unbox_##type(type boxed) { return unbox_VkType<type>(this, boxed); } \
    type BoxedHandleManager::try_unbox_##type(type boxed) {                                       \
        return try_unbox_VkType<type>(this, boxed);                                               \
    }                                                                                             \
    void BoxedHandleManager::delete_##type(type boxed) {                                          \
        return delete_VkType<type>(this, boxed);                                                  \
    }                                                                                             \
    type BoxedHandleManager::unboxed_to_boxed_non_dispatchable_##type(type unboxed) {             \
        return unboxed_to_boxed_non_dispatchable_VkType<type>(this, unboxed);                     \
    }                                                                                             \
    void BoxedHandleManager::delayed_delete_##type(type boxed, VkDevice device,                   \
                                                   std::function<void()> callback) {              \
        delayed_delete_VkType<type>(this, boxed, device, std::move(callback));                    \
    }                                                                                             \
    void BoxedHandleManager::set_boxed_non_dispatchable_##type(type boxed, type underlying) {     \
        set_boxed_non_dispatchable_VkType<type>(this, boxed, underlying);                         \
    }

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEFINE)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEFINE)

}  // namespace vk
}  // namespace gfxstream
