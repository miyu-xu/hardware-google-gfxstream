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

#pragma once

#include <deque>
#include <mutex>

#include "VulkanDispatch.h"
#include "VulkanHandles.h"
#include "VulkanStream.h"
#include "aemu/base/ThreadAnnotations.h"
#include "aemu/base/containers/HybridEntityManager.h"
#include "aemu/base/containers/Lookup.h"
#include "aemu/base/synchronization/ConditionVariable.h"
#include "aemu/base/synchronization/Lock.h"

namespace gfxstream {
namespace vk {

#define DEFINE_BOXED_HANDLE_TYPE_TAG(type) Tag_##type,

enum BoxedHandleTypeTag {
    Tag_Invalid = 0,

    GOLDFISH_VK_LIST_HANDLE_TYPES_BY_STAGE(DEFINE_BOXED_HANDLE_TYPE_TAG)

    // additional generic tag
    Tag_VkGeneric = 1001,
};

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
using BoxedHandle = uint64_t;
using UnboxedHandle = uint64_t;

struct OrderMaintenanceInfo {
    uint32_t sequenceNumber = 0;
    android::base::Lock lock;
    android::base::ConditionVariable cv;

    uint32_t refcount = 1;

    void incRef() { __atomic_add_fetch(&refcount, 1, __ATOMIC_SEQ_CST); }

    bool decRef() { return 0 == __atomic_sub_fetch(&refcount, 1, __ATOMIC_SEQ_CST); }
};

static void acquireOrderMaintInfo(OrderMaintenanceInfo* ord) {
    if (!ord) return;
    ord->incRef();
}

static void releaseOrderMaintInfo(OrderMaintenanceInfo* ord) {
    if (!ord) return;
    if (ord->decRef()) delete ord;
}

class BoxedHandleInfo {
   public:
    UnboxedHandle underlying;
    VulkanDispatch* dispatch = nullptr;
    bool ownDispatch = false;
    OrderMaintenanceInfo* ordMaintInfo = nullptr;
    VulkanMemReadingStream* readStream = nullptr;
};

class BoxedHandleManager {
   public:
    // The hybrid entity manager uses a sequence lock to protect access to
    // a working set of 16000 handles, allowing us to avoid using a regular
    // lock for those. Performance is degraded when going over this number,
    // as it will then fall back to a std::map.
    //
    // We use 16000 as the max number of live handles to track; we don't
    // expect the system to go over 16000 total live handles, outside some
    // dEQP object management tests.
    using Store = android::base::HybridEntityManager<16000, BoxedHandle, BoxedHandleInfo>;

    BoxedHandle add(const BoxedHandleInfo& item, BoxedHandleTypeTag tag);

    void update(BoxedHandle handle, const BoxedHandleInfo& item, BoxedHandleTypeTag tag);

    void remove(BoxedHandle h);
    void removeDelayed(uint64_t h, VkDevice device, std::function<void()> callback);

    // Do not call directly! Instead use `processDelayedRemovesForDevice()` which has
    // thread safety annotations for `VkDecoderGlobalState::Impl`.
    void processDelayedRemoves(VkDevice device);

    BoxedHandleInfo* get(BoxedHandle handle);
    BoxedHandle getBoxedFromUnboxed(UnboxedHandle unboxed);

    void replayHandles(std::vector<BoxedHandle> handles);

    void clear();

    template <typename VkObjectT>
        VkObjectT new_boxed_VkType(VkObjectT underlying, bool dispatchable = false, VulkanDispatch* dispatch = nullptr, bool ownsDispatch = false)
{
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
    return (VkObjectT)add(info, GetTag<VkObjectT>());
}

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DECL(type)                                 \
    type new_boxed_##type(type underlying, VulkanDispatch* dispatch, bool ownDispatch);

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DECL(type)                                  \
    type new_boxed_non_dispatchable_##type(type underlying);

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DECL)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DECL)

   private:
    mutable Store mStore;

    std::mutex mMutex;
    std::unordered_map<UnboxedHandle, BoxedHandle> mReverseMap GUARDED_BY(mMutex);

    struct DelayedRemove {
        BoxedHandle handle;
        std::function<void()> callback;
    };
    std::unordered_map<VkDevice, std::vector<DelayedRemove>> mDelayedRemoves GUARDED_BY(mMutex);

    // If true, `add()` will use and consume the handles in `mHandleReplayQueue`.
    // This is useful for snapshot loading when a explicit set of handles should
    // be used when replaying commands.
    bool mHandleReplay = false;
    std::deque<BoxedHandle> mHandleReplayQueue;
};

//extern BoxedHandleManager sBoxedHandleManager;

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DECL(type)                                 \
    void delete_##type(type boxed);                                                     \
    type unbox_##type(type boxed);                                                      \
    type try_unbox_##type(type boxed);                                                  \
    type unboxed_to_boxed_##type(type boxed);                                           \
    VulkanDispatch* dispatch_##type(type boxed);                                        \
    OrderMaintenanceInfo* ordmaint_##type(type boxed);                                  \
    VulkanMemReadingStream* readstream_##type(type boxed);

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DECL(type)                                  \
    type new_boxed_non_dispatchable_##type(type underlying);                                 \
    void delete_##type(type boxed);                                                          \
    void delayed_delete_##type(type boxed, VkDevice device, std::function<void()> callback); \
    type unbox_##type(type boxed);                                                           \
    type try_unbox_##type(type boxed);                                                       \
    type unboxed_to_boxed_non_dispatchable_##type(type boxed);                               \
    void set_boxed_non_dispatchable_##type(type boxed, type underlying);

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DECL)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DECL)

}  // namespace vk
}  // namespace gfxstream
