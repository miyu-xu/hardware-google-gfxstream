// Copyright (C) 2020 The Android Open Source Project
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <log/log.h>

#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <variant>


#include "common/goldfish_vk_dispatch.h"
#include "gfxstream/guest/vulkan/vulkan_for_testing.h"
#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"
#include "virgl_hw.h"
#include "render-utils/virtio-gpu-gfxstream-renderer.h"

#include "VirtGpu.h"

namespace gfxstream {
namespace {

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;
using testing::Eq;
using testing::NotNull;
using vk::VulkanDispatch;

using VirtioGpuResourceId = uint32_t;

// For now, single guest process testing.
constexpr const uint32_t kVirtioGpuContextId = 1;

class TestingVirtGpuBlobMapping : public VirtGpuBlobMapping {
  public:
    TestingVirtGpuBlobMapping(VirtGpuBlobPtr blob, uint8_t* mapped)
        : mBlob(blob), mMapped(mapped) {}

    ~TestingVirtGpuBlobMapping(void) {
        stream_renderer_resource_unmap(mBlob->getResourceHandle());
    }

    uint8_t* asRawPtr(void) override { return mMapped; }

  private:
    VirtGpuBlobPtr mBlob;
    uint8_t* mMapped;
};

class TestingVirtGpuBlob : public std::enable_shared_from_this<TestingVirtGpuBlob>, public VirtGpuBlob {
  public:
    TestingVirtGpuBlob(uint32_t resourceId) : mResourceId(resourceId) {}

    ~TestingVirtGpuBlob() {
        stream_renderer_resource_unref(mResourceId);
    }

    VirtGpuBlobMappingPtr createMapping(void) override {
        void* mapped = nullptr;

        int ret = stream_renderer_resource_map(mResourceId, &mapped, nullptr);
        if (ret) {
            ERR("Failed to map resource:%d", mResourceId);
            return nullptr;
        }

        return std::make_shared<TestingVirtGpuBlobMapping>(shared_from_this(), reinterpret_cast<uint8_t*>(mapped));
    }

    uint32_t getResourceHandle(void) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    uint32_t getBlobHandle(void) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int exportBlob(struct VirtGpuExternalHandle& handle) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int wait(void) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

  private:
    uint32_t mResourceId;
};

class TestingVirtGpuDevice : public VirtGpuDevice {
  public:
    TestingVirtGpuDevice() : mVirtioGpuTaskProcessingThread([this]() { RunVirtioGpuTaskProcessingLoop(); }) {
        const std::string name = "test";
        stream_renderer_context_create(kVirtioGpuContextId, name.size(), name.data(), 0);
    }

    ~TestingVirtGpuDevice() {
        mShuttingDown = true;
    }

    int64_t getDeviceHandle() override { return -1; }

    VirtGpuBlobPtr createBlob(const struct VirtGpuCreateBlob& blobCreate) override {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        VirtioGpuTaskCreateBlob task{
            .resourceId = resourceId,
            .params = {
                .blob_mem = static_cast<uint32_t>(blobCreate.blobMem),
                .blob_flags = static_cast<uint32_t>(blobCreate.flags),
                .blob_id = blobCreate.blobId,
                .size = blobCreate.size,
            },
        };
        EnqueueVirtioGpuTask(std::move(task));

        return std::make_shared<TestingVirtGpuBlob>(resourceId);
    }

    VirtGpuBlobPtr createPipeBlob(uint32_t size) {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        VirtioGpuTaskCreateResource task{
            .resourceId = resourceId,
            .params = {
                .handle = resourceId,
                .target = /*PIPE_BUFFER=*/0,
                .format = VIRGL_FORMAT_R8G8B8A8_UNORM,
                .bind = VIRGL_BIND_CUSTOM,
                .width = size,
                .height = 1,
                .depth = 1,
                .array_size = 0,
                .last_level = 0,
                .nr_samples = 0,
                .flags = 0,
            },
        };
        EnqueueVirtioGpuTask(std::move(task));

        return std::make_shared<TestingVirtGpuBlob>(resourceId);
    }

    int execBuffer(struct VirtGpuExecBuffer& execbuffer, VirtGpuBlobPtr blob) {
        if (execbuffer.flags & kFenceOut) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
            return 0;
        }

        return 0;
    }

    VirtGpuBlobPtr importBlob(const struct VirtGpuExternalHandle& handle) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return nullptr;
    }

  private:
    struct VirtioGpuTaskCreateBlob {
        uint32_t resourceId;
        struct stream_renderer_create_blob params;
    };
    struct VirtioGpuTaskCreateResource {
        uint32_t resourceId;
        struct stream_renderer_resource_create_args params;
    };
    struct VirtioGpuTaskExecBuffer {
        uint32_t resourceId;
    };
    using VirtioGpuTask = std::variant<VirtioGpuTaskCreateBlob,
                                       VirtioGpuTaskCreateResource,
                                       VirtioGpuTaskExecBuffer>;

    void EnqueueVirtioGpuTask(VirtioGpuTask task) {
        std::lock_guard<std::mutex> lock(mVirtioGpuTaskMutex);
        mVirtioGpuTasks.push(task);
    }

    void DoTask(VirtioGpuTaskCreateBlob task) {
        int ret = stream_renderer_create_blob(kVirtioGpuContextId, task.resourceId, &task.params, nullptr, 0, nullptr);
        if (ret) {
            ERR("Failed to create blob.");
        }
    }

    void DoTask(VirtioGpuTaskCreateResource task) {
        int ret = stream_renderer_resource_create(&task.params, nullptr, 0);
        if (ret) {
            ERR("Failed to create pipe blob.");
        }
    }

    void DoTask(VirtioGpuTaskExecBuffer task) {

    }

    void RunVirtioGpuTaskProcessingLoop() {
        while (!mShuttingDown.load()) {
            std::optional<VirtioGpuTask> task;

            {
                std::lock_guard<std::mutex> lock(mVirtioGpuTaskMutex);
                if (!mVirtioGpuTasks.empty()) {
                    task = std::move(mVirtioGpuTasks.front());
                    mVirtioGpuTasks.pop();
                }
            }

            if (task) {
                std::visit(
                    [this](auto&& work){
                        using T = std::decay_t<decltype(work)>;
                        if constexpr (std::is_same_v<T, VirtioGpuTaskCreateBlob>) {
                            DoTask(std::move(work));
                        } else if constexpr (std::is_same_v<T, VirtioGpuTaskCreateResource>) {
                            DoTask(std::move(work));
                        } else if constexpr (std::is_same_v<T, VirtioGpuTaskExecBuffer>) {
                            DoTask(std::move(work));
                        }
                    }, *task);
            }
        }
    }

    std::atomic<uint32_t> mNextVirtioGpuResourceId{1};

    std::atomic_bool mShuttingDown{false};

    std::mutex mVirtioGpuTaskMutex;
    std::queue<VirtioGpuTask> mVirtioGpuTasks;

    std::thread mVirtioGpuTaskProcessingThread;
};

class TestingAHB {
  public:
    ~TestingAHB() {
        stream_renderer_resource_unref(mResourceId);
    }

  private:
    friend class TestingAHB;

    TestingAHB(uint32_t resource_id) : mResourceId(resource_id) {}

    uint32_t mResourceId = 0;
};

static void* sGuestVulkanDispatchDlOpen() {
    // Random non-null value to make satisfy
    // vk::init_vulkan_dispatch_from_system_loader().
    static void* sFakeLibraryHandle = (void*)(new int());
    return sFakeLibraryHandle;
}

static void* sGuestVulkanDispatchDlSym(void*, const char* symbol) {
    auto procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        GfxstreamGuestVulkanGetInstanceProcAddrForTesting());
    return (void*)procAddr(VK_NULL_HANDLE, symbol);
}

VulkanDispatch GetGuestVulkanDispatch() {
    static VulkanDispatch* sGuestDispatch = [](){
        VulkanDispatch* dispatch = new VulkanDispatch();

        vk::init_vulkan_dispatch_from_system_loader(sGuestVulkanDispatchDlOpen,
                                                    sGuestVulkanDispatchDlSym,
                                                    dispatch);

        return dispatch;
    }();
    return *sGuestDispatch;
}

void WriteFence(void* cookie, struct stream_renderer_fence* fence) {}

class AndroidIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::vector<stream_renderer_param> renderer_params{
            {STREAM_RENDERER_PARAM_USER_DATA, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(this))},
            {STREAM_RENDERER_PARAM_FENCE_CALLBACK, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&WriteFence))},
            {STREAM_RENDERER_PARAM_RENDERER_FLAGS, STREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT},
            {STREAM_RENDERER_PARAM_WIN0_WIDTH, 32},
            {STREAM_RENDERER_PARAM_WIN0_HEIGHT, 32},
        };
        ASSERT_THAT(stream_renderer_init(renderer_params.data(), renderer_params.size()), Eq(0));

        mGuest.vk = GetGuestVulkanDispatch();
    }

    void TearDown() override {
        stream_renderer_teardown();
    }

    struct Guest {
       VulkanDispatch vk;
    } mGuest;
};

TEST_F(AndroidIntegrationTest, VulkanImportAHB) {
    auto& guest = mGuest;

    //auto ahb = guest.gralloc.Allocate(32, 32);

    /*
    const VkExternalMemoryImageCreateInfo external_mem_create_info = {
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
    };
    const VkImageCreateInfo image_create_info = {
        .pNext = &external_mem_create_info,
    };

    VkImage vk_image = VK_NULL_HANDLE;
    guest.vk.vkCreateImage(..., &image_create_info, ..., &vk_image);

    const VkImportAndroidHardwareBufferInfoANDROID import_ahb_info = {
        .buffer = ahb,
    };
    const VkMemoryDedicatedAllocateInfo mem_dedicated_alloc_info = {
        .pNext  = &import_ahb_info,
        .image  = vk_image,
    };

    const VkMemoryAllocateInfo mem_alloc_info = {};

    VkDeviceMemory vk_image_mem = VK_NULL_HANDLE
    guest.vk.vkAllocateMemory(..., &mem_alloc_info, ..., &vk_image_mem);
    guest.vk.vkBindImageMemory(..., vk_image, vk_image_mem, ...);

    // Render with AHB.
    */
    ASSERT_THAT(guest.vk.vkCreateInstance, NotNull());
}

}  // namespace
}  // namespace gfxstream