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

#include <vulkan/vulkan.h>

#include "common/goldfish_vk_dispatch.h"
#include "drm_fourcc.h"
#include "gfxstream/guest/vulkan/vulkan_for_testing.h"
#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"
#include "virgl_hw.h"
#include "render-utils/virtio-gpu-gfxstream-renderer.h"

#include "Gralloc.h"
#include "HostConnection.h"
#include "VirtGpu.h"



#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

// For a function:
//
//   android::base::expected<vk::Type, vk::Result> Foo();
//
// simplifies
//
//   auto obj_expect = Foo();
//   if (!obj_expect.ok()) {
//     return expect.error();
//   }
//   auto obj = std::move(obj.value());
//
// to
//
//   auto obj = VK_EXPECT(Foo());
#define VK_EXPECT(x)                                    \
  ({                                                    \
    auto vk_expect_android_base_expected = (x);         \
    if (!vk_expect_android_base_expected.ok()) {        \
      ASSERT_THAT(vk_expect_android_base_expected.ok(), ::testing::IsTrue()); \
    };                                                  \
    std::move(vk_expect_android_base_expected.value()); \
  })

#define VK_EXPECT_RESULT(x)                             \
  ({                                                    \
    auto vk_expect_android_base_expected = (x);         \
    if (!vk_expect_android_base_expected.ok()) {        \
      return vk_expect_android_base_expected.error();   \
    };                                                  \
    std::move(vk_expect_android_base_expected.value()); \
  })

#define VK_RETURN_IF_NOT_SUCCESS(x)                    \
  do {                                                 \
    vk::Result result = (x);                           \
    if (result != vk::Result::eSuccess) return result; \
  } while (0);

#define VK_RETURN_UNEXPECTED_IF_NOT_SUCCESS(x)  \
  do {                                          \
    vk::Result result = (x);                    \
    if (result != vk::Result::eSuccess) {       \
      return android::base::unexpected(result); \
    }                                           \
  } while (0);

#define VK_ASSERT(x)                                                          \
  do {                                                                        \
    if (vk::Result result = (x); result != vk::Result::eSuccess) {            \
      LOG(FATAL) << __FILE__ << ":" << __LINE__ << ":" << __PRETTY_FUNCTION__ \
                 << ": " << #x << " returned " << to_string(result);          \
    }                                                                         \
  } while (0);

template <typename VkType>
using VkExpected = android::base::expected<VkType, vk::Result>;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace gfxstream {
namespace {

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;
using testing::Eq;
using testing::Gt;
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
    static std::shared_ptr<TestingVirtGpuResource> createBlob(uint32_t resourceId,
                                                              std::shared_ptr<TestingVirtGpuDevice> device,
                                                              std::shared_future<void> createCompleted) {
        return std::shared_ptr<TestingVirtGpuResource>(
            new TestingVirtGpuResource(resourceId, ResourceType::kBlob, device, createCompleted));
    }

    static std::shared_ptr<TestingVirtGpuResource> createPipe(uint32_t resourceId,
                                                              std::shared_ptr<TestingVirtGpuDevice> device,
                                                              std::shared_future<void> createCompleted,
                                                              std::unique_ptr<uint8_t[]> resourceBytes) {
        return std::shared_ptr<TestingVirtGpuResource>(
            new TestingVirtGpuResource(resourceId, ResourceType::kPipe, device, createCompleted, std::move(resourceBytes)));
    }

    ~TestingVirtGpuResource() {
        stream_renderer_resource_unref(mResourceId);
    }

    VirtGpuBlobMappingPtr createMapping(void) override {
        if (mResourceType == ResourceType::kBlob) {
            void* mapped = nullptr;

            int ret = stream_renderer_resource_map(mResourceId, &mapped, nullptr);
            if (ret) {
                ERR("Failed to map resource:%d", mResourceId);
                return nullptr;
            }

            return std::make_shared<TestingVirtGpuBlobMapping>(shared_from_this(), reinterpret_cast<uint8_t*>(mapped));
        } else if (mResourceType == ResourceType::kPipe) {
            return std::make_shared<TestingVirtGpuBlobMapping>(shared_from_this(), reinterpret_cast<uint8_t*>(mResourceGuestBytes.get()));
        } else {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unhandled.";
        }
    }

    uint32_t getResourceHandle() override {
        return mResourceId;
    }

    uint32_t getBlobHandle() override {
        if (mResourceType != ResourceType::kBlob) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Attempting to get blob handle for non-blob resource";
        }

        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return 0;
    }

    int exportBlob(VirtGpuExternalHandle& handle) override {
        if (mResourceType != ResourceType::kBlob) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Attempting to export blob for non-blob resource";
        }

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

    int transferFromHost(uint32_t size) override;
    int transferToHost(uint32_t size) override;

  private:
    enum class ResourceType {
        kBlob,
        kPipe,
    };

    TestingVirtGpuResource(uint32_t resourceId,
                           ResourceType resourceType,
                           std::shared_ptr<TestingVirtGpuDevice> device,
                           std::shared_future<void> createCompleted,
                           std::unique_ptr<uint8_t[]> resourceBytes = nullptr)
        : mResourceId(resourceId),
          mResourceType(resourceType),
          mDevice(device),
          mPendingCommandWaitables({createCompleted}),
          mResourceGuestBytes(std::move(resourceBytes)) {}

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

    const uint32_t mResourceId;
    const ResourceType mResourceType;
    const std::shared_ptr<TestingVirtGpuDevice> mDevice;

    std::mutex mPendingCommandWaitablesMutex;
    std::vector<std::shared_future<void>> mPendingCommandWaitables;

    // For non-blob resources, the guest shadow memory.
    std::unique_ptr<uint8_t[]> mResourceGuestBytes;
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
        return TestingVirtGpuResource::createBlob(resourceId, shared_from_this(), taskCompletedWaitable);
    }

    VirtGpuBlobPtr createPipeBlob(uint32_t size) override {
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
        return TestingVirtGpuResource::createPipe(resourceId, shared_from_this(), taskCompletedWaitable, std::move(resourceBytes));
    }

    VirtGpuBlobPtr createTexture(uint32_t width,
                                 uint32_t height,
                                 uint32_t format) {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        // TODO: calculate for real.
        const uint32_t resourceSize = width * height * 4;

        auto resourceBytes = std::make_unique<uint8_t[]>(resourceSize);

        if (format != DRM_FORMAT_ABGR8888) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        }

        VirtioGpuTaskCreateResource task{
            .resourceId = resourceId,
            .resourceBytes = resourceBytes.get(),
            .params = {
                .handle = resourceId,
                .target = /*PIPE_TEXTURE_2D=*/2,
                .format = VIRGL_FORMAT_R8G8B8A8_UNORM,
                .bind = VIRGL_BIND_CUSTOM,
                .width = width,
                .height = height,
                .depth = 1,
                .array_size = 1,
                .last_level = 0,
                .nr_samples = 0,
                .flags = 0,
            },
        };

        auto taskCompletedWaitable = EnqueueVirtioGpuTask(std::move(task));
        return TestingVirtGpuResource::createPipe(resourceId, shared_from_this(), taskCompletedWaitable, std::move(resourceBytes));
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
            if (auto* b = dynamic_cast<TestingVirtGpuResource*>(blob.get()); b != nullptr) {
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
    if (mResourceType != ResourceType::kPipe) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Unexpected transferFromHost() called on non-pipe resource.";
    }

    std::shared_future<void> transferCompleteWaitable = mDevice->transferFromHost(mResourceId, size);

    {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
        mPendingCommandWaitables.push_back(transferCompleteWaitable);
    }

    return 0;
}

int TestingVirtGpuResource::transferToHost(uint32_t size) {
    if (mResourceType != ResourceType::kPipe) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Unexpected transferFromHost() called on non-pipe resource.";
    }

    std::shared_future<void> transferCompleteWaitable = mDevice->transferToHost(mResourceId, size);

    {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
        mPendingCommandWaitables.push_back(transferCompleteWaitable);
    }

    return 0;
}

class TestingAHB {
  public:
    TestingAHB(std::shared_ptr<TestingVirtGpuResource> resource)
        : mResource(resource) {}

    uint32_t getResourceId() const {
        return mResource->getResourceHandle();
    }

    int getAndroidFormat() const {
        return /*AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM=*/1;
    }

    uint32_t getDrmFormat() const {
        return DRM_FORMAT_ABGR8888;
    }

    AHardwareBuffer* asAHardwareBuffer() {
        return reinterpret_cast<AHardwareBuffer*>(this);
    }

  private:
    std::shared_ptr<TestingVirtGpuResource> mResource;
};

class TestingVirtGpuGralloc : public Gralloc {
   public:
    TestingVirtGpuGralloc(std::shared_ptr<TestingVirtGpuDevice> device)
        : mDevice(device) {}

    uint32_t createColorBuffer(renderControl_client_context_t*, int, int, uint32_t) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented.";
    }

    int allocate(uint32_t width,
                 uint32_t height,
                 uint32_t format,
                 uint64_t usage,
                 AHardwareBuffer** outputAhb) override {
        (void)width;
        (void)height;
        (void)format;
        (void)usage;
        (void)outputAhb;

        // TODO: support export flow?
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented.";

        return 0;
    }

    std::unique_ptr<TestingAHB> allocate(uint32_t width,
                                         uint32_t height,
                                         uint32_t format) {
        auto resource = mDevice->createTexture(width, height, format);
        if (!resource) {
            return nullptr;
        }

        resource->wait();

        auto resourceTyped = std::dynamic_pointer_cast<TestingVirtGpuResource>(resource);
        if (!resourceTyped) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Failed to dynamic cast virtio gpu resource.";
        }

        return std::make_unique<TestingAHB>(std::move(resourceTyped));
    }

    void acquire(AHardwareBuffer* ahb) override {
        // TODO
    }

    void release(AHardwareBuffer* ahb) override {
        // TODO
    }

    uint32_t getHostHandle(const native_handle_t* handle) override {
        const auto* testingAhb = reinterpret_cast<const TestingAHB*>(handle);
        return testingAhb->getResourceId();
    }

    uint32_t getHostHandle(const AHardwareBuffer* handle) override {
        const auto* testingAhb = reinterpret_cast<const TestingAHB*>(handle);
        return testingAhb->getResourceId();
    }

    int getFormat(const native_handle_t* handle) override {
        const auto* testingAhb = reinterpret_cast<const TestingAHB*>(handle);
        return testingAhb->getAndroidFormat();
    }

    int getFormat(const AHardwareBuffer* handle) override {
        const auto* testingAhb = reinterpret_cast<const TestingAHB*>(handle);
        return testingAhb->getAndroidFormat();
    }

    uint32_t getFormatDrmFourcc(const AHardwareBuffer* handle) override {
        const auto* testingAhb = reinterpret_cast<const TestingAHB*>(handle);
        return testingAhb->getDrmFormat();
    }

    size_t getAllocatedSize(const AHardwareBuffer*) override {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented.";
        return 0;
    }

  private:
    std::shared_ptr<TestingVirtGpuDevice> mDevice;
};

static void* sGuestVulkanDispatchDlOpen() {
    // Random non-null value to make
    // vk::init_vulkan_dispatch_from_system_loader() happy.
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
                STREAM_RENDERER_FLAGS_USE_VK_BIT},
            {STREAM_RENDERER_PARAM_WIN0_WIDTH, 32},
            {STREAM_RENDERER_PARAM_WIN0_HEIGHT, 32},
        };
        ASSERT_THAT(stream_renderer_init(renderer_params.data(), renderer_params.size()), Eq(0));

        const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
        stream_renderer_context_create(kVirtioGpuContextId, name.size(), name.data(), 0);

        mVirtioGpuDevice = std::make_shared<TestingVirtGpuDevice>();
        setVirtioGpuDeviceInstanceForTesting(mVirtioGpuDevice.get());

        mVirtioGpuGralloc = std::make_shared<TestingVirtGpuGralloc>(mVirtioGpuDevice);

        mGuest.gralloc = mVirtioGpuGralloc.get();
        mGuest.vk = GetGuestVulkanDispatch();

        HostConnection::get()->setGrallocHelperForTesting(mVirtioGpuGralloc.get());
    }

    void TearDown() override {
        ALOGE("TearDown");

        HostConnection::exit();

        // Figure out more reliable way for guest shutdown to complete...
        std::this_thread::sleep_for(std::chrono::seconds(3));

        stream_renderer_context_destroy(kVirtioGpuContextId);

        stream_renderer_teardown();
    }

    struct Guest {
        TestingVirtGpuGralloc* gralloc;
        VulkanDispatch vk;
    } mGuest;

  private:
    std::shared_ptr<TestingVirtGpuDevice> mVirtioGpuDevice;
    std::shared_ptr<TestingVirtGpuGralloc> mVirtioGpuGralloc;
};

TEST_F(AndroidIntegrationTest, VulkanImportAHB) {
    auto& guest = mGuest;

    ::vk::DynamicLoader loader;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            GfxstreamGuestVulkanGetInstanceProcAddrForTesting()));

    ::vk::raii::Context context;

    const auto availableInstanceLayers = context.enumerateInstanceLayerProperties();
    ALOGE("Available instance layers:");
    for (const ::vk::LayerProperties& layer : availableInstanceLayers) {
        ALOGE(" - %s", layer.layerName.data());
    }

    constexpr const bool kEnableValidationLayers = false;

    std::vector<const char*> requestedInstanceExtensions;
    std::vector<const char*> requestedInstanceLayers;
    if (kEnableValidationLayers) {
        requestedInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const ::vk::ApplicationInfo applicationInfo{
        .pApplicationName = ::testing::UnitTest::GetInstance()->current_test_info()->name(),
        .applicationVersion = 1,
        .pEngineName = "Gfxstream Testing Engine",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };
    const ::vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(requestedInstanceLayers.size()),
        .ppEnabledLayerNames = requestedInstanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requestedInstanceExtensions.size()),
        .ppEnabledExtensionNames = requestedInstanceExtensions.data(),
    };
    auto instance = VK_EXPECT(::vk::raii::Instance::create(context, instanceCreateInfo));

    auto physicalDevices = VK_EXPECT(::vk::raii::PhysicalDevices::create(instance));

    ALOGE("Available physical devices:");
    for (const auto& physicalDevice : physicalDevices) {
        const auto physicalDeviceProps = physicalDevice.getProperties();
        ALOGE(" - %s", physicalDeviceProps.deviceName.data());
    }

    ::vk::raii::PhysicalDevice physicalDevice = std::move(physicalDevices[0]);
    {
        const auto physicalDeviceProps = physicalDevice.getProperties();
        ALOGE("Selected physical device: %s", physicalDeviceProps.deviceName.data());
    }
    {
        const auto exts = physicalDevice.enumerateDeviceExtensionProperties();
        ALOGE("Available physical device extensions:");
        for (const auto& ext : exts) {
            ALOGE(" - %s", ext.extensionName.data());
        }
    }

    uint32_t graphicsQueueFamilyIndex = -1;
    {
        const auto props = physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < props.size(); i++) {
            const auto& prop = props[i];
            if (prop.queueFlags & ::vk::QueueFlagBits::eGraphics) {
                graphicsQueueFamilyIndex = i;
                break;
            }
        }
    }
    ASSERT_THAT(graphicsQueueFamilyIndex, Not(Eq(-1)));

    const float queuePriority = 1.0f;
    const ::vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    const ::vk::DeviceCreateInfo deviceCreateInfo = {
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .queueCreateInfoCount = 1,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };
    auto device = VK_EXPECT(::vk::raii::Device::create(physicalDevice, deviceCreateInfo));


    const uint32_t width = 32;
    const uint32_t height = 32;
    auto ahb = guest.gralloc->allocate(width, height, DRM_FORMAT_ABGR8888);

    const ::vk::ImageCreateInfo imageCreateInfo = {
        .imageType = ::vk::ImageType::e2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = ::vk::Format::eR8G8B8A8Unorm,
        .tiling = ::vk::ImageTiling::eOptimal,
        .initialLayout = ::vk::ImageLayout::eUndefined,
        .usage = ::vk::ImageUsageFlagBits::eSampled |
                 ::vk::ImageUsageFlagBits::eTransferDst |
                 ::vk::ImageUsageFlagBits::eTransferSrc,
        .sharingMode = ::vk::SharingMode::eExclusive,
        .samples = ::vk::SampleCountFlagBits::e1,
    };
    auto image = VK_EXPECT(::vk::raii::Image::create(device, imageCreateInfo));
    auto imageMemoryRequirements = image.getMemoryRequirements();

    // TODO
    const uint32_t imageMemoryIndex = 0;

    const ::vk::MemoryAllocateInfo imageMemoryAllocateInfo = {
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryIndex,
    };
    auto imageMemory = VK_EXPECT(::vk::raii::DeviceMemory::create(device, imageMemoryAllocateInfo));
    image.bindMemory(*imageMemory, 0);
}

}  // namespace
}  // namespace gfxstream