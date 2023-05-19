// Copyright (C) 2023 The Android Open Source Project
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

#include <future>
#include <inttypes.h>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log/log.h>

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
using testing::Not;
using testing::NotNull;
using vk::VulkanDispatch;

using VirtioGpuResourceId = uint32_t;

// For now, single guest process testing.
constexpr const uint32_t kVirtioGpuContextId = 1;

class TestingVirtGpuDevice;

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

class TestingVirtGpuResource : public std::enable_shared_from_this<TestingVirtGpuResource>, public VirtGpuBlob {
  public:
    TestingVirtGpuResource(uint32_t resourceId,
                           std::unique_ptr<uint8_t[]> resourceBytes,
                           std::shared_ptr<TestingVirtGpuDevice> device,
                           std::shared_future<void> createCompleted)
        : mResourceId(resourceId),
          mResourceGuestBytes(std::move(resourceBytes)),
          mDevice(device),
          mPendingCommandWaitables({createCompleted}) {}

    ~TestingVirtGpuResource() {
        stream_renderer_resource_unref(mResourceId);
    }

    VirtGpuBlobMappingPtr createMapping(void) override {
        return std::make_shared<TestingVirtGpuBlobMapping>(shared_from_this(), reinterpret_cast<uint8_t*>(mResourceGuestBytes.get()));
    }

    uint32_t getResourceHandle() override {
        return mResourceId;
    }

    uint32_t getBlobHandle() override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Attempting to get blob handle for non-blob resource";
        return 0;
    }

    int exportBlob(VirtGpuExternalHandle& handle) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Attempting to export blob for non-blob resource";
        return 0;
    }

    int wait() override {
        std::vector<std::shared_future<void>> currentPendingCommandWaitables;

        {
            std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
            currentPendingCommandWaitables = mPendingCommandWaitables;
            mPendingCommandWaitables.clear();
        }

        for (auto& waitable : currentPendingCommandWaitables) {
            waitable.wait();
        }

        return 0;
    }

    int transferFromHost(uint32_t size) override;
    int transferToHost(uint32_t size) override;

  private:
    uint32_t mResourceId;
    std::unique_ptr<uint8_t[]> mResourceGuestBytes;
    std::shared_ptr<TestingVirtGpuDevice> mDevice;
    std::mutex mPendingCommandWaitablesMutex;
    std::vector<std::shared_future<void>> mPendingCommandWaitables;
};

class TestingVirtGpuBlob : public std::enable_shared_from_this<TestingVirtGpuBlob>, public VirtGpuBlob {
  public:
    TestingVirtGpuBlob(uint32_t resourceId,
                       std::shared_future<void> createCompleted)
        : mResourceId(resourceId),
          mPendingCommandWaitables({createCompleted}) {}

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

    uint32_t getResourceHandle() override {
        return mResourceId;
    }

    uint32_t getBlobHandle() override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int exportBlob(VirtGpuExternalHandle& handle) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int wait() override {
        std::vector<std::shared_future<void>> currentPendingCommandWaitables;

        {
            std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
            currentPendingCommandWaitables = mPendingCommandWaitables;
            mPendingCommandWaitables.clear();
        }

        for (auto& waitable : currentPendingCommandWaitables) {
            waitable.wait();
        }

        return 0;
    }

    int transferToHost(uint32_t size) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int transferFromHost(uint32_t size) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

  private:
    friend class TestingVirtGpuDevice;

    void addPendingCommandWaitable(std::shared_future<void> waitable) {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);

        mPendingCommandWaitables.erase(
            std::remove_if(mPendingCommandWaitables.begin(),
                           mPendingCommandWaitables.end(),
                           [](const std::shared_future<void>& waitable) {
                                return waitable.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                           }),
            mPendingCommandWaitables.end());

        mPendingCommandWaitables.push_back(std::move(waitable));
    }

    uint32_t mResourceId;

    std::mutex mPendingCommandWaitablesMutex;
    std::vector<std::shared_future<void>> mPendingCommandWaitables;
};

class TestingVirtGpuDevice : public std::enable_shared_from_this<TestingVirtGpuDevice>, public VirtGpuDevice {
  public:
    TestingVirtGpuDevice()
        : mVirtioGpuTaskProcessingThread([this]() { RunVirtioGpuTaskProcessingLoop(); }) {}

    ~TestingVirtGpuDevice() {
        mShuttingDown = true;
        mVirtioGpuTaskProcessingThread.join();
    }

    int64_t getDeviceHandle() override { return -1; }

    VirtGpuCaps getCaps() override {
         VirtGpuCaps caps = {
            .params = {
                [kParam3D] = 1,
                [kParamCapsetFix] = 1,
                [kParamResourceBlob] = 1,
                [kParamHostVisible] = 1,
                [kParamCrossDevice] = 0,
                [kParamContextInit] = 1,
                [kParamSupportedCapsetIds] = 0,
                [kParamCreateGuestHandle] = 0,
            },
        };

        stream_renderer_fill_caps(0, 0, &caps.gfxstreamCapset);

        return caps;
    }

    VirtGpuBlobPtr createBlob(const struct VirtGpuCreateBlob& blobCreate) override {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        ALOGE("Enquing task to create blob resource-id:%d size:%" PRIu64, resourceId, blobCreate.size);

        VirtioGpuTaskCreateBlob task{
            .resourceId = resourceId,
            .params = {
                .blob_mem = static_cast<uint32_t>(blobCreate.blobMem),
                .blob_flags = static_cast<uint32_t>(blobCreate.flags),
                .blob_id = blobCreate.blobId,
                .size = blobCreate.size,
            },
        };
        auto taskCompletedWaitable = EnqueueVirtioGpuTask(std::move(task));
        return std::make_shared<TestingVirtGpuBlob>(resourceId, taskCompletedWaitable);
    }

    VirtGpuBlobPtr createPipeBlob(uint32_t size) {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        auto resourceBytes = std::make_unique<uint8_t[]>(size);

        VirtioGpuTaskCreateResource task{
            .resourceId = resourceId,
            .resourceBytes = resourceBytes.get(),
            .params = {
                .handle = resourceId,
                .target = /*PIPE_BUFFER=*/0,
                .format = VIRGL_FORMAT_R8_UNORM,
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
        auto taskCompletedWaitable = EnqueueVirtioGpuTask(std::move(task));
        return std::make_shared<TestingVirtGpuResource>(resourceId, std::move(resourceBytes), shared_from_this(), taskCompletedWaitable);
    }

    int execBuffer(struct VirtGpuExecBuffer& execbuffer, VirtGpuBlobPtr blob) {
        if (execbuffer.flags & kFenceOut) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
            return 0;
        }

        VirtioGpuTaskExecBuffer task = {};

        task.commandBuffer.resize(execbuffer.command_size);
        std::memcpy(task.commandBuffer.data(), execbuffer.command, execbuffer.command_size);

        auto taskCompletedWaitable = EnqueueVirtioGpuTask(std::move(task));

        if (blob) {
            if (auto* b = dynamic_cast<TestingVirtGpuBlob*>(blob.get()); b != nullptr) {
                b->addPendingCommandWaitable(std::move(taskCompletedWaitable));
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                    << "Execbuffer called with non-blob resource.";
            }
        }

        return 0;
    }

    VirtGpuBlobPtr importBlob(const struct VirtGpuExternalHandle& handle) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return nullptr;
    }

  private:
    friend class TestingVirtGpuResource;

    std::shared_future<void> transferFromHost(uint32_t resourceId, uint32_t transferSize) {
        VirtioGpuTaskTransferFromHost task = {
            .resourceId = resourceId,
            .transferSize = transferSize,
        };
        return EnqueueVirtioGpuTask(std::move(task));
    }

    std::shared_future<void> transferToHost(uint32_t resourceId, uint32_t transferSize) {
        VirtioGpuTaskTransferToHost task = {
            .resourceId = resourceId,
            .transferSize = transferSize,
        };
        return EnqueueVirtioGpuTask(std::move(task));
    }

  private:
    struct VirtioGpuTaskCreateBlob {
        uint32_t resourceId;
        struct stream_renderer_create_blob params;
    };
    struct VirtioGpuTaskCreateResource {
        uint32_t resourceId;
        uint8_t* resourceBytes;
        struct stream_renderer_resource_create_args params;

    };
    struct VirtioGpuTaskExecBuffer {
        std::vector<std::byte> commandBuffer;
    };
    struct VirtioGpuTaskTransferToHost {
        uint32_t resourceId;
        uint32_t transferSize;
    };
    struct VirtioGpuTaskTransferFromHost {
        uint32_t resourceId;
        uint32_t transferSize;
    };
    using VirtioGpuTask = std::variant<VirtioGpuTaskCreateBlob,
                                       VirtioGpuTaskCreateResource,
                                       VirtioGpuTaskExecBuffer,
                                       VirtioGpuTaskTransferFromHost,
                                       VirtioGpuTaskTransferToHost>;

    struct VirtioGpuTaskWithWaitable {
        VirtioGpuTask task;
        std::promise<void> taskCompletedSignaler;
    };

    std::shared_future<void> EnqueueVirtioGpuTask(VirtioGpuTask task) {
        std::promise<void> taskCompletedSignaler;
        std::shared_future<void> taskCompletedWaitable(taskCompletedSignaler.get_future());

        std::lock_guard<std::mutex> lock(mVirtioGpuTaskMutex);
        mVirtioGpuTasks.push(
            VirtioGpuTaskWithWaitable{
                .task = std::move(task),
                .taskCompletedSignaler = std::move(taskCompletedSignaler),
            });

        return taskCompletedWaitable;
    }

    void DoTask(VirtioGpuTaskCreateBlob task) {
        ALOGE("Performing task to create blob resource-id:%d", task.resourceId);

        int ret = stream_renderer_create_blob(kVirtioGpuContextId, task.resourceId, &task.params, nullptr, 0, nullptr);
        if (ret) {
            ALOGE("Failed to create blob.");
        }

        ALOGE("Performing task to create blob resource-id:%d - done", task.resourceId);
    }

    void DoTask(VirtioGpuTaskCreateResource task) {
        ALOGE("Performing task to create resource resource-id:%d", task.resourceId);

        int ret = stream_renderer_resource_create(&task.params, nullptr, 0);
        if (ret) {
            ALOGE("Failed to create resource:%d", task.resourceId);
        }

        struct iovec iov = {
            .iov_base = task.resourceBytes,
            .iov_len = task.params.width,
        };
        ret = stream_renderer_resource_attach_iov(task.resourceId, &iov, 1);
        if (ret) {
            ALOGE("Failed to attach iov to resource:%d", task.resourceId);
        }

        ALOGE("Performing task to create resource resource-id:%d - done", task.resourceId);

        stream_renderer_ctx_attach_resource(kVirtioGpuContextId, task.resourceId);
    }

    void DoTask(VirtioGpuTaskExecBuffer task) {
        ALOGE("Performing task to execbuffer");

        if (task.commandBuffer.size() % 4 != 0) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unaligned command?";
        }

        int ret = stream_renderer_submit_cmd(task.commandBuffer.data(),
                                             kVirtioGpuContextId,
                                             task.commandBuffer.size() / 4);
        if (ret) {
            ALOGE("Failed to execbuffer.");
        }

        ALOGE("Performing task to execbuffer - done");
    }

    void DoTask(VirtioGpuTaskTransferFromHost task) {
        struct stream_renderer_box transferBox = {
            .x = 0,
            .y = 0,
            .z = 0,
            .w = task.transferSize,
            .h = 1,
            .d = 1,
        };

        int ret = stream_renderer_transfer_read_iov(task.resourceId,
                                                     kVirtioGpuContextId,
                                                     /*level=*/0,
                                                     /*stride=*/0,
                                                     /*layer_stride=*/0,
                                                     &transferBox,
                                                     /*offset=*/0,
                                                     /*iov=*/nullptr,
                                                     /*iovec_cnt=*/0);
        if (ret) {
            ALOGE("Failed to transferFromHost() for resource:%" PRIu32, task.resourceId);
        }
    }

    void DoTask(VirtioGpuTaskTransferToHost task) {
        struct stream_renderer_box transferBox = {
            .x = 0,
            .y = 0,
            .z = 0,
            .w = task.transferSize,
            .h = 1,
            .d = 1,
        };

        int ret = stream_renderer_transfer_write_iov(task.resourceId,
                                                     kVirtioGpuContextId,
                                                     /*level=*/0,
                                                     /*stride=*/0,
                                                     /*layer_stride=*/0,
                                                     &transferBox,
                                                     /*offset=*/0,
                                                     /*iov=*/nullptr,
                                                     /*iovec_cnt=*/0);
        if (ret) {
            ALOGE("Failed to transferToHost() for resource:%" PRIu32, task.resourceId);
        }
    }

    void DoTask(VirtioGpuTaskWithWaitable task) {
        std::visit(
            [this](auto&& work){
                using T = std::decay_t<decltype(work)>;
                if constexpr (std::is_same_v<T, VirtioGpuTaskCreateBlob>) {
                    DoTask(std::move(work));
                } else if constexpr (std::is_same_v<T, VirtioGpuTaskCreateResource>) {
                    DoTask(std::move(work));
                } else if constexpr (std::is_same_v<T, VirtioGpuTaskExecBuffer>) {
                    DoTask(std::move(work));
                }  else if constexpr (std::is_same_v<T, VirtioGpuTaskTransferFromHost>) {
                    DoTask(std::move(work));
                }  else if constexpr (std::is_same_v<T, VirtioGpuTaskTransferToHost>) {
                    DoTask(std::move(work));
                }
            }, task.task);

        task.taskCompletedSignaler.set_value();
    }

    void RunVirtioGpuTaskProcessingLoop() {
        while (!mShuttingDown.load()) {
            std::optional<VirtioGpuTaskWithWaitable> task;

            {
                std::lock_guard<std::mutex> lock(mVirtioGpuTaskMutex);
                if (!mVirtioGpuTasks.empty()) {
                    task = std::move(mVirtioGpuTasks.front());
                    mVirtioGpuTasks.pop();
                }
            }

            if (task) {
                DoTask(std::move(*task));
            }
        }
    }

    std::atomic<uint32_t> mNextVirtioGpuResourceId{1};

    std::atomic_bool mShuttingDown{false};

    std::mutex mVirtioGpuTaskMutex;
    std::queue<VirtioGpuTaskWithWaitable> mVirtioGpuTasks;
    std::thread mVirtioGpuTaskProcessingThread;
};

int TestingVirtGpuResource::transferFromHost(uint32_t size) {
    std::shared_future<void> transferCompleteWaitable = mDevice->transferFromHost(mResourceId, size);

    {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
        mPendingCommandWaitables.push_back(transferCompleteWaitable);
    }

    return 0;
}

int TestingVirtGpuResource::transferToHost(uint32_t size) {
    std::shared_future<void> transferCompleteWaitable = mDevice->transferToHost(mResourceId, size);

    {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
        mPendingCommandWaitables.push_back(transferCompleteWaitable);
    }

    return 0;
}

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
            {STREAM_RENDERER_PARAM_RENDERER_FLAGS,
                STREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT |
                STREAM_RENDERER_FLAGS_USE_VK_BIT |
                STREAM_RENDERER_FLAGS_USE_GLES_BIT},
            {STREAM_RENDERER_PARAM_WIN0_WIDTH, 32},
            {STREAM_RENDERER_PARAM_WIN0_HEIGHT, 32},
        };
        ASSERT_THAT(stream_renderer_init(renderer_params.data(), renderer_params.size()), Eq(0));

        const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
        stream_renderer_context_create(kVirtioGpuContextId, name.size(), name.data(), 0);

        mVirtioGpuDevice = std::make_shared<TestingVirtGpuDevice>();
        setVirtioGpuDeviceInstanceForTesting(mVirtioGpuDevice.get());

        ALOGE("jasonjason setting up guest vulkan dispatch table");
        mGuest.vk = GetGuestVulkanDispatch();
        ALOGE("jasonjason setting up guest vulkan dispatch table - done");
    }

    void TearDown() override {
        stream_renderer_context_destroy(kVirtioGpuContextId);

        stream_renderer_teardown();
    }

    struct Guest {
       VulkanDispatch vk;
    } mGuest;

  private:
    std::shared_ptr<TestingVirtGpuDevice> mVirtioGpuDevice;
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

    const VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = nullptr,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };
    VkInstance instance = VK_NULL_HANDLE;
    ASSERT_THAT(guest.vk.vkCreateInstance(&ici, nullptr, &instance), Eq(VK_SUCCESS));
    ASSERT_THAT(instance, Not(Eq(VK_NULL_HANDLE)));
}

}  // namespace
}  // namespace gfxstream