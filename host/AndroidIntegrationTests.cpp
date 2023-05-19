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

#include <filesystem>
#include <future>
#include <inttypes.h>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <variant>

#include <dlfcn.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log/log.h>

#include <vulkan/vulkan.h>

#include "aemu/base/system/System.h"
#include "common/goldfish_vk_dispatch.h"
#include "drm_fourcc.h"
#include "gfxstream/guest/egl/egl_for_testing.h"
#include "gfxstream/guest/vulkan/vulkan_for_testing.h"
#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"
#include "virgl_hw.h"
#include "render-utils/virtio-gpu-gfxstream-renderer.h"

#include "Gralloc.h"
#include "HostConnection.h"
#include "ProcessPipe.h"
#include "VirtGpu.h"


#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

using GenericFnType = void*(void);
using GetProcAddrType = GenericFnType*(const char*);

bool SetupGuestDispatches(gfxstream::gl::EGLDispatch* egl,
                          gfxstream::gl::GLESv2Dispatch* gles2) {
    auto getAddr = reinterpret_cast<GetProcAddrType*>(GfxstreamGuestEglGetProcAddrForTesting());
    if (!getAddr) {
        ALOGE("Invalid eglGetProcAddr().");
        return false;
    }

#define LOAD_EGL_FUNCTION(return_type, function_name, signature) \
    egl-> function_name = reinterpret_cast< return_type (*) signature >(getAddr( #function_name ));

    LIST_RENDER_EGL_FUNCTIONS(LOAD_EGL_FUNCTION)
    LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(LOAD_EGL_FUNCTION)

    const std::filesystem::path testDirectory = android::base::getProgramDirectory();
    const std::string gles2LibPath = (testDirectory / "libGLESv2_emulation.so").string();
    void* gles2Lib = dlopen(gles2LibPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!gles2Lib) {
        ALOGE("Failed to load Gfxstream GLES2 library from %s.", gles2LibPath.c_str());
        return false;
    }

#define LOAD_GLES2_FUNCTION(return_type, function_name, signature, callargs)    \
    gles2-> function_name = reinterpret_cast< return_type (*) signature >(getAddr( #function_name )); \
    if (!gles2-> function_name) { \
        gles2-> function_name = reinterpret_cast< return_type (*) signature >(dlsym(gles2Lib, #function_name)); \
    }

    LIST_GLES2_FUNCTIONS(LOAD_GLES2_FUNCTION, LOAD_GLES2_FUNCTION)

    return true;
}


#define VULKAN_HPP_NAMESPACE vkhpp
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
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
    vkhpp::Result result = (x);                        \
    if (result != vkhpp::Result::eSuccess) return result; \
  } while (0);

#define VK_RETURN_UNEXPECTED_IF_NOT_SUCCESS(x)  \
  do {                                          \
    vkhpp::Result result = (x);                 \
    if (result != vkhpp::Result::eSuccess) {       \
      return android::base::unexpected(result); \
    }                                           \
  } while (0);

#define VK_ASSERT(x)                                                          \
  do {                                                                        \
    if (vkhpp::Result result = (x); result != vkhpp::Result::eSuccess) {      \
      LOG(FATAL) << __FILE__ << ":" << __LINE__ << ":" << __PRETTY_FUNCTION__ \
                 << ": " << #x << " returned " << to_string(result);          \
    }                                                                         \
  } while (0);

template <typename VkType>
using VkExpected = android::base::expected<VkType, vkhpp::Result>;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace gfxstream {
namespace {

using namespace std::chrono_literals;

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;
using testing::Eq;
using testing::Gt;
using testing::IsTrue;
using testing::Not;
using testing::NotNull;
using vk::VulkanDispatch;

// For now, single guest process testing.
constexpr const uint32_t kVirtioGpuContextId = 1;




std::optional<uint32_t> GlFormatToDrmFormat(uint32_t glFormat) {
    switch (glFormat) {
        case GL_RGB565:
            return DRM_FORMAT_BGR565;
        case GL_RGBA:
            return DRM_FORMAT_ABGR8888;
    }
    return std::nullopt;
}

std::optional<uint32_t> DrmFormatToVirglFormat(uint32_t drmFormat) {
    switch (drmFormat) {
        case DRM_FORMAT_BGR888:
        case DRM_FORMAT_RGB888:
            return VIRGL_FORMAT_R8G8B8_UNORM;
        case DRM_FORMAT_XRGB8888:
            return VIRGL_FORMAT_B8G8R8X8_UNORM;
        case DRM_FORMAT_ARGB8888:
            return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DRM_FORMAT_XBGR8888:
            return VIRGL_FORMAT_R8G8B8X8_UNORM;
        case DRM_FORMAT_ABGR8888:
            return VIRGL_FORMAT_R8G8B8A8_UNORM;
        case DRM_FORMAT_ABGR2101010:
            return VIRGL_FORMAT_R10G10B10A2_UNORM;
        case DRM_FORMAT_BGR565:
            return VIRGL_FORMAT_B5G6R5_UNORM;
        case DRM_FORMAT_R8:
            return VIRGL_FORMAT_R8_UNORM;
        case DRM_FORMAT_R16:
            return VIRGL_FORMAT_R16_UNORM;
        case DRM_FORMAT_RG88:
            return VIRGL_FORMAT_R8G8_UNORM;
        case DRM_FORMAT_NV12:
            return VIRGL_FORMAT_NV12;
        case DRM_FORMAT_NV21:
            return VIRGL_FORMAT_NV21;
        case DRM_FORMAT_YVU420:
            return VIRGL_FORMAT_YV12;
    }
    return std::nullopt;
}


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
        ALOGE("Unref resource:%d", (int)mResourceId);
        stream_renderer_resource_unref(mResourceId);
    }

    VirtGpuBlobMappingPtr createMapping(void) override {
        if (mResourceType == ResourceType::kBlob) {
            void* mapped = nullptr;

            int ret = stream_renderer_resource_map(mResourceId, &mapped, nullptr);
            if (ret) {
                ALOGE("Failed to map resource:%d", mResourceId);
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

    int transferFromHost(uint32_t offset, uint32_t size) override;
    int transferToHost(uint32_t offset, uint32_t size) override;

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
                                 uint32_t drmFormat) {
        const uint32_t resourceId = mNextVirtioGpuResourceId++;

        // TODO: calculate for real.
        const uint32_t resourceSize = width * height * 4;

        auto resourceBytes = std::make_unique<uint8_t[]>(resourceSize);

        auto virglFormat = DrmFormatToVirglFormat(drmFormat);
        if (!virglFormat) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unhandled format:" << drmFormat;
        }

        VirtioGpuTaskCreateResource task{
            .resourceId = resourceId,
            .resourceBytes = resourceBytes.get(),
            .params = {
                .handle = resourceId,
                .target = /*PIPE_TEXTURE_2D=*/2,
                .format = *virglFormat,
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
        std::optional<uint32_t> fence;

        if (execbuffer.flags & kFenceOut) {
            fence = CreateEmulatedFence();
        }

        VirtioGpuTaskExecBuffer task = {};

        task.commandBuffer.resize(execbuffer.command_size);
        std::memcpy(task.commandBuffer.data(), execbuffer.command, execbuffer.command_size);

        auto taskCompletedWaitable = EnqueueVirtioGpuTask(std::move(task), fence);

        if (blob) {
            if (auto* b = dynamic_cast<TestingVirtGpuResource*>(blob.get()); b != nullptr) {
                b->addPendingCommandWaitable(std::move(taskCompletedWaitable));
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                    << "Execbuffer called with non-blob resource.";
            }
        }

        if (execbuffer.flags & kFenceOut) {
            execbuffer.handle.osHandle = *fence;
            execbuffer.handle.type = kFenceHandleSyncFd;
        }

        return 0;
    }

    VirtGpuBlobPtr importBlob(const struct VirtGpuExternalHandle& handle) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unimplemented";
        return nullptr;
    }

  private:
    friend class TestingVirtGpuResource;

    std::shared_future<void> transferFromHost(uint32_t resourceId, uint32_t transferOffset, uint32_t transferSize) {
        VirtioGpuTaskTransferFromHost task = {
            .resourceId = resourceId,
            .transferOffset = transferOffset,
            .transferSize = transferSize,
        };
        return EnqueueVirtioGpuTask(std::move(task));
    }

    std::shared_future<void> transferToHost(uint32_t resourceId, uint32_t transferOffset, uint32_t transferSize) {
        VirtioGpuTaskTransferToHost task = {
            .resourceId = resourceId,
            .transferOffset = transferOffset,
            .transferSize = transferSize,
        };
        return EnqueueVirtioGpuTask(std::move(task));
    }

  private:
    friend class TestingVirtGpuSyncHelper;

    int WaitOnEmulatedFence(int fenceAsFileDescriptor, int timeoutMilliseconds) {
        uint32_t fenceId = static_cast<uint32_t>(fenceAsFileDescriptor);
        ALOGE("Waiting on fence:%d", (int)fenceId);

        std::shared_future<void> waitable;

        {
            std::lock_guard<std::mutex> lock(mVirtioGpuFencesMutex);

            auto fenceIt = mVirtioGpuFences.find(fenceId);
            if (fenceIt == mVirtioGpuFences.end()) {
                ALOGE("Fence:%d already signaled", (int)fenceId);
                return 0;
            }
            auto& fence = fenceIt->second;

            waitable = fence.waitable;
        }

        auto status = waitable.wait_for(std::chrono::milliseconds(timeoutMilliseconds));
        if (status == std::future_status::ready) {
            ALOGE("Finished waiting for fence:%d", (int)fenceId);
            return 0;
        } else {
            ALOGE("Timed out waiting for fence:%d", (int)fenceId);
            return -1;
        }
    }

  public:
    // Public for callback from Gfxstream.
    void SignalEmulatedFence(uint32_t fenceId) {
        ALOGE("Signaling fence:%d", (int)fenceId);

        std::lock_guard<std::mutex> lock(mVirtioGpuFencesMutex);

        auto fenceIt = mVirtioGpuFences.find(fenceId);
        if (fenceIt == mVirtioGpuFences.end()) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Failed to find fence:" << fenceId;
        }
        auto& fenceInfo = fenceIt->second;
        fenceInfo.signaler.set_value();
    }

  private:
    uint32_t CreateEmulatedFence() {
        uint32_t fenceId = mNextVirtioGpuFenceId++;

        ALOGE("Creating fence:%d", (int)fenceId);

        std::lock_guard<std::mutex> lock(mVirtioGpuFencesMutex);

        auto [fenceIt, fenceCreated] = mVirtioGpuFences.emplace(fenceId, EmulatedFence{});
        if (!fenceCreated) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Attempting to recreate fence:" << fenceId;
        }

        auto& fenceInfo = fenceIt->second;
        fenceInfo.waitable = fenceInfo.signaler.get_future();

        return fenceId;
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
        uint32_t transferOffset;
        uint32_t transferSize;
    };
    struct VirtioGpuTaskTransferFromHost {
        uint32_t resourceId;
        uint32_t transferOffset;
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
        std::optional<uint32_t> fence;
    };

    std::shared_future<void> EnqueueVirtioGpuTask(VirtioGpuTask task, std::optional<uint32_t> fence = std::nullopt) {
        std::promise<void> taskCompletedSignaler;
        std::shared_future<void> taskCompletedWaitable(taskCompletedSignaler.get_future());

        std::lock_guard<std::mutex> lock(mVirtioGpuTaskMutex);
        mVirtioGpuTasks.push(
            VirtioGpuTaskWithWaitable{
                .task = std::move(task),
                .taskCompletedSignaler = std::move(taskCompletedSignaler),
                .fence = fence,
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
            .x = task.transferOffset,
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
            .x = task.transferOffset,
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

        if (task.fence) {
            const stream_renderer_fence fenceInfo = {
                .flags = STREAM_RENDERER_FLAG_FENCE_RING_IDX,
                .fence_id = *task.fence,
                .ctx_id = kVirtioGpuContextId,
                .ring_idx = 0,
            };
            int ret = stream_renderer_create_fence(&fenceInfo);
            if (ret) {
                ALOGE("Failed to create fence.");
            }
        }

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
    std::atomic<uint32_t> mNextVirtioGpuFenceId{1};

    std::atomic_bool mShuttingDown{false};

    std::mutex mVirtioGpuTaskMutex;
    std::queue<VirtioGpuTaskWithWaitable> mVirtioGpuTasks;
    std::thread mVirtioGpuTaskProcessingThread;

    struct EmulatedFence {
        std::promise<void> signaler;
        std::shared_future<void> waitable;
    };
    std::mutex mVirtioGpuFencesMutex;
    std::unordered_map<uint32_t, EmulatedFence> mVirtioGpuFences;
};

int TestingVirtGpuResource::transferFromHost(uint32_t offset, uint32_t size) {
    if (mResourceType != ResourceType::kPipe) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Unexpected transferFromHost() called on non-pipe resource.";
    }

    std::shared_future<void> transferCompleteWaitable = mDevice->transferFromHost(mResourceId, offset, size);

    {
        std::lock_guard<std::mutex> lock(mPendingCommandWaitablesMutex);
        mPendingCommandWaitables.push_back(transferCompleteWaitable);
    }

    return 0;
}

int TestingVirtGpuResource::transferToHost(uint32_t offset, uint32_t size) {
    if (mResourceType != ResourceType::kPipe) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Unexpected transferFromHost() called on non-pipe resource.";
    }

    std::shared_future<void> transferCompleteWaitable = mDevice->transferToHost(mResourceId, offset, size);

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

    uint32_t createColorBuffer(renderControl_client_context_t*, int width, int height, uint32_t glFormat) {
        auto drmFormat = GlFormatToDrmFormat(glFormat);
        if (!drmFormat) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << "Unhandled format:" << glFormat;
        }

        auto ahb = allocate(width, height, *drmFormat);

        uint32_t hostHandle = ahb->getResourceId();
        mAllocatedColorBuffers.emplace(hostHandle, std::move(ahb));
        return hostHandle;
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
    std::unordered_map<uint32_t, std::unique_ptr<TestingAHB>> mAllocatedColorBuffers;

    std::shared_ptr<TestingVirtGpuDevice> mDevice;
};

class TestingVirtGpuSyncHelper : public SyncHelper {
  public:
    TestingVirtGpuSyncHelper(std::shared_ptr<TestingVirtGpuDevice> device)
        : mDevice(device) {}


    int wait(int syncFd, int timeoutMilliseconds) override {
        return mDevice->WaitOnEmulatedFence(syncFd, timeoutMilliseconds);
    }

    int close(int) override {
        return 0;
    }

  private:
    std::shared_ptr<TestingVirtGpuDevice> mDevice;
};

void WriteFence(void* cookie, struct stream_renderer_fence* fence) {
    auto* device = reinterpret_cast<TestingVirtGpuDevice*>(cookie);
    device->SignalEmulatedFence(fence->fence_id);
}

struct TestParams {
    bool with_gl;
    bool with_vk;

    std::string ToString() const {
        std::string ret;
        ret += (with_gl ? "With" : "Without");
        ret += "Gl";
        ret += (with_vk ? "With" : "Without");
        ret += "Vk";
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const TestParams& params) {
        return os << params.ToString();
    }
};

std::string GetTestName(const ::testing::TestParamInfo<TestParams>& info) {
    return info.param.ToString();
}

class GfxstreamEnd2EndTest : public ::testing::TestWithParam<TestParams> {
  protected:
    void SetUp() override {
        const TestParams params = GetParam();

        mDevice = std::make_shared<TestingVirtGpuDevice>();
        setVirtioGpuDeviceInstanceForTesting(mDevice.get());

        std::vector<stream_renderer_param> renderer_params{
            stream_renderer_param{
                .key = STREAM_RENDERER_PARAM_USER_DATA,
                .value = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(mDevice.get())),
            },
            stream_renderer_param{
                .key = STREAM_RENDERER_PARAM_FENCE_CALLBACK,
                .value = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&WriteFence)),
            },
            stream_renderer_param{
                .key = STREAM_RENDERER_PARAM_RENDERER_FLAGS,
                .value =
                    static_cast<uint64_t>(STREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT) |
                    (params.with_gl ? static_cast<uint64_t>(STREAM_RENDERER_FLAGS_USE_GLES_BIT) : 0 ) |
                    (params.with_vk ? static_cast<uint64_t>(STREAM_RENDERER_FLAGS_USE_VK_BIT) : 0 ),
            },
            stream_renderer_param{
                .key = STREAM_RENDERER_PARAM_WIN0_WIDTH,
                .value = 32,
            },
            stream_renderer_param{
                .key = STREAM_RENDERER_PARAM_WIN0_HEIGHT,
                .value = 32,
            },
        };
        ASSERT_THAT(stream_renderer_init(renderer_params.data(), renderer_params.size()), Eq(0));

        const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
        stream_renderer_context_create(kVirtioGpuContextId, name.size(), name.data(), 0);

        // TODO:
        HostConnection::getOrCreate(VIRTIO_GPU_CAPSET_GFXSTREAM);

        mGralloc = std::make_unique<TestingVirtGpuGralloc>(mDevice);
        HostConnection::get()->setGrallocHelperForTesting(mGralloc.get());

        mSync = std::make_unique<TestingVirtGpuSyncHelper>(mDevice);
        HostConnection::get()->setSyncHelperForTesting(mSync.get());

        if (params.with_gl) {
            mEgl = std::make_unique<gfxstream::gl::EGLDispatch>();
            mGles2 = std::make_unique<gfxstream::gl::GLESv2Dispatch>();
            ASSERT_TRUE(SetupGuestDispatches(mEgl.get(), mGles2.get()));
        }
        if (params.with_vk) {
            mVk = std::make_unique<vkhpp::raii::Context>(
                reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                    GfxstreamGuestVulkanGetInstanceProcAddrForTesting()));
        }
    }

    void TearDownGuest() {
        mGralloc.reset();

        mGles2.reset();

        if (mEgl) {
            EGLDisplay display = mEgl->eglGetCurrentContext();
            if (display != EGL_NO_DISPLAY) {
                mEgl->eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }
            mEgl->eglReleaseThread();
            mEgl.reset();
        }

        mVk.reset();

        HostConnection::exit();
        processPipeRestart();

        mDevice.reset();
        mSync.reset();

        // Figure out more reliable way for guest shutdown to complete...
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    void TearDownHost() {
        stream_renderer_context_destroy(kVirtioGpuContextId);
        stream_renderer_teardown();
    }

    void TearDown() override {
        TearDownGuest();
        TearDownHost();
    }

    std::shared_ptr<TestingVirtGpuDevice> mDevice;
    std::unique_ptr<TestingVirtGpuGralloc> mGralloc;
    std::unique_ptr<TestingVirtGpuSyncHelper> mSync;
    std::unique_ptr<gfxstream::gl::EGLDispatch> mEgl;
    std::unique_ptr<gfxstream::gl::GLESv2Dispatch> mGles2;
    std::unique_ptr<vkhpp::raii::Context> mVk;
};


class GfxstreamEnd2EndGlTest : public GfxstreamEnd2EndTest {};
class GfxstreamEnd2EndVkTest : public GfxstreamEnd2EndTest {};

namespace {

uint32_t GetMemoryType(const vkhpp::raii::PhysicalDevice& physicalDevice,
                       const vkhpp::MemoryRequirements& memoryRequirements,
                       vkhpp::MemoryPropertyFlags memoryProperties) {
  const auto props = physicalDevice.getMemoryProperties();
  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (!(memoryRequirements.memoryTypeBits & (1 << i))) {
      continue;
    }
    if ((props.memoryTypes[i].propertyFlags & memoryProperties) != memoryProperties) {
      continue;
    }
    return i;
  }
  return -1;
}

template <typename DurationType>
constexpr uint64_t AsVkTimeout(DurationType duration) {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

}  // namespace

TEST_P(GfxstreamEnd2EndVkTest, VulkanImportAHB) {
    const auto availableInstanceLayers = mVk->enumerateInstanceLayerProperties();
    ALOGE("Available instance layers:");
    for (const vkhpp::LayerProperties& layer : availableInstanceLayers) {
        ALOGE(" - %s", layer.layerName.data());
    }

    constexpr const bool kEnableValidationLayers = false;

    std::vector<const char*> requestedInstanceExtensions;
    std::vector<const char*> requestedInstanceLayers;
    if (kEnableValidationLayers) {
        requestedInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const vkhpp::ApplicationInfo applicationInfo{
        .pApplicationName = ::testing::UnitTest::GetInstance()->current_test_info()->name(),
        .applicationVersion = 1,
        .pEngineName = "Gfxstream Testing Engine",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };
    const vkhpp::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(requestedInstanceLayers.size()),
        .ppEnabledLayerNames = requestedInstanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requestedInstanceExtensions.size()),
        .ppEnabledExtensionNames = requestedInstanceExtensions.data(),
    };
    auto instance = VK_EXPECT(vkhpp::raii::Instance::create(*mVk, instanceCreateInfo));

    auto physicalDevices = VK_EXPECT(vkhpp::raii::PhysicalDevices::create(instance));

    ALOGE("Available physical devices:");
    for (const auto& physicalDevice : physicalDevices) {
        const auto physicalDeviceProps = physicalDevice.getProperties();
        ALOGE(" - %s", physicalDeviceProps.deviceName.data());
    }

    vkhpp::raii::PhysicalDevice physicalDevice = std::move(physicalDevices[0]);
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
            if (prop.queueFlags & vkhpp::QueueFlagBits::eGraphics) {
                graphicsQueueFamilyIndex = i;
                break;
            }
        }
    }
    ASSERT_THAT(graphicsQueueFamilyIndex, Not(Eq(-1)));

    const float queuePriority = 1.0f;
    const vkhpp::DeviceQueueCreateInfo deviceQueueCreateInfo = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    const std::vector<const char*> deviceExtensions = {
        VK_ANDROID_NATIVE_BUFFER_EXTENSION_NAME,
    };
    const vkhpp::DeviceCreateInfo deviceCreateInfo = {
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .queueCreateInfoCount = 1,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    auto device = VK_EXPECT(vkhpp::raii::Device::create(physicalDevice, deviceCreateInfo));
    auto queue = vkhpp::raii::Queue(device, graphicsQueueFamilyIndex, 0);

    const uint32_t width = 32;
    const uint32_t height = 32;
    auto ahb = mGralloc->allocate(width, height, DRM_FORMAT_ABGR8888);

    const vkhpp::NativeBufferANDROID imageNativeBufferInfo = {
        .handle = (const uint32_t*)ahb->asAHardwareBuffer(),
    };

    const vkhpp::ImageCreateInfo imageCreateInfo = {
        .pNext = &imageNativeBufferInfo,
        .imageType = vkhpp::ImageType::e2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = vkhpp::Format::eR8G8B8A8Unorm,
        .tiling = vkhpp::ImageTiling::eOptimal,
        .initialLayout = vkhpp::ImageLayout::eUndefined,
        .usage = vkhpp::ImageUsageFlagBits::eSampled |
                 vkhpp::ImageUsageFlagBits::eTransferDst |
                 vkhpp::ImageUsageFlagBits::eTransferSrc,
        .sharingMode = vkhpp::SharingMode::eExclusive,
        .samples = vkhpp::SampleCountFlagBits::e1,
    };
    auto image = VK_EXPECT(vkhpp::raii::Image::create(device, imageCreateInfo));
    auto imageMemoryRequirements = image.getMemoryRequirements();

    const uint32_t imageMemoryIndex =
        GetMemoryType(physicalDevice, imageMemoryRequirements, vkhpp::MemoryPropertyFlagBits::eDeviceLocal);
    ASSERT_THAT(imageMemoryIndex, Not(Eq(-1)));

    const vkhpp::MemoryAllocateInfo imageMemoryAllocateInfo = {
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryIndex,
    };
    auto imageMemory = VK_EXPECT(vkhpp::raii::DeviceMemory::create(device, imageMemoryAllocateInfo));
    image.bindMemory(*imageMemory, 0);

    const vkhpp::BufferCreateInfo bufferCreateInfo = {
        .size = static_cast<VkDeviceSize>(12 * 1024 * 1024),
        .usage = vkhpp::BufferUsageFlagBits::eTransferDst |
                 vkhpp::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vkhpp::SharingMode::eExclusive,
    };
    auto stagingBuffer = VK_EXPECT(vkhpp::raii::Buffer::create(device, bufferCreateInfo));

    const auto stagingBufferMemoryRequirements = stagingBuffer.getMemoryRequirements();
    const auto stagingBufferMemoryType =
         GetMemoryType(physicalDevice, stagingBufferMemoryRequirements,
                       vkhpp::MemoryPropertyFlagBits::eHostVisible |
                       vkhpp::MemoryPropertyFlagBits::eHostCoherent);

    const vkhpp::MemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    auto stagingBufferMemory = VK_EXPECT(vkhpp::raii::DeviceMemory::create(device, stagingBufferMemoryAllocateInfo));
    stagingBuffer.bindMemory(*stagingBufferMemory, 0);

    const vkhpp::CommandPoolCreateInfo commandPoolCreateInfo = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
    };
    auto commandPool = VK_EXPECT(vkhpp::raii::CommandPool::create(device, commandPoolCreateInfo));

    const vkhpp::CommandBufferAllocateInfo commandBufferAllocateInfo = {
        .level = vkhpp::CommandBufferLevel::ePrimary,
        .commandPool = *commandPool,
        .commandBufferCount = 1,
    };
    auto commandBuffers = VK_EXPECT(vkhpp::raii::CommandBuffers::create(device, commandBufferAllocateInfo));
    auto commandBuffer = std::move(commandBuffers[0]);

    const vkhpp::CommandBufferBeginInfo commandBufferBeginInfo = {
        .flags = vkhpp::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    commandBuffer.begin(commandBufferBeginInfo);

    commandBuffer.end();

    std::vector<vkhpp::CommandBuffer> commandBufferHandles;
    commandBufferHandles.push_back(*commandBuffer);

    auto transferFence = VK_EXPECT(vkhpp::raii::Fence::create(device, vkhpp::FenceCreateInfo()));

    vkhpp::SubmitInfo submitInfo = {
        .commandBufferCount = static_cast<uint32_t>(commandBufferHandles.size()),
        .pCommandBuffers = commandBufferHandles.data(),
    };
    queue.submit(submitInfo, *transferFence);

    auto waitResult = device.waitForFences(*transferFence, VK_TRUE, AsVkTimeout(3s));
    ASSERT_THAT(waitResult, Eq(vkhpp::Result::eSuccess));

    std::vector<vkhpp::Semaphore> semaphores;
    int fence = queue.signalReleaseImageANDROID(semaphores, *image);
    ASSERT_THAT(fence, Not(Eq(-1)));

    ASSERT_THAT(mSync->wait(fence, 3000), Eq(0));
}

TEST_P(GfxstreamEnd2EndGlTest, Egl) {
    EGLDisplay display = mEgl->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ASSERT_THAT(display, Not(Eq(EGL_NO_DISPLAY)));

    int versionMajor = 0;
    int versionMinor = 0;
    ASSERT_THAT(mEgl->eglInitialize(display, &versionMajor, &versionMinor), IsTrue());

    ASSERT_THAT(mEgl->eglBindAPI(EGL_OPENGL_ES_API), IsTrue());

    // clang-format off
    static const EGLint configAttributes[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };
    // clang-format on

    int numConfigs = 0;
    ASSERT_THAT(mEgl->eglChooseConfig(display, configAttributes, nullptr, 1, &numConfigs), IsTrue());
    ASSERT_THAT(numConfigs, Gt(0));

    EGLConfig config = nullptr;
    ASSERT_THAT(mEgl->eglChooseConfig(display, configAttributes, &config, 1, &numConfigs), IsTrue());
    ASSERT_THAT(config, Not(Eq(nullptr)));

    constexpr const int width = 32;
    constexpr const int height = 32;

    // clang-format off
    static const EGLint surfaceAttributes[] = {
        EGL_WIDTH,  width,
        EGL_HEIGHT, height,
        EGL_NONE,
    };
    // clang-format on

    EGLSurface surface = mEgl->eglCreatePbufferSurface(display, config, surfaceAttributes);
    ASSERT_THAT(surface, Not(Eq(EGL_NO_SURFACE)));

    // clang-format off
    static const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };
    // clang-format on

    EGLContext context = mEgl->eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    ASSERT_THAT(context, Not(Eq(EGL_NO_CONTEXT)));

    ASSERT_THAT(mEgl->eglMakeCurrent(display, surface, surface, context), IsTrue());

    // https://cs.android.com/android/platform/superproject/+/refs/heads/master:frameworks/native/opengl/libs/EGL/egl_platform_entries.cpp;l=1157
    ASSERT_THAT(SetupGuestDispatches(mEgl.get(), mGles2.get()), IsTrue());

    const std::string extensionString = (const char*)mGles2->glGetString(GL_EXTENSIONS);
    ALOGE("GL extensions: %s", extensionString.c_str());

    ASSERT_THAT(mEgl->eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT), IsTrue());
    ASSERT_THAT(mEgl->eglDestroyContext(display, context), IsTrue());
    ASSERT_THAT(mEgl->eglDestroySurface(display, surface), IsTrue());
}

INSTANTIATE_TEST_CASE_P(GfxstreamEnd2EndTests, GfxstreamEnd2EndGlTest,
                        ::testing::ValuesIn({
                            TestParams{
                                .with_gl = true,
                                .with_vk = false,
                            },
                        }),
                        &GetTestName);

INSTANTIATE_TEST_CASE_P(GfxstreamEnd2EndTests, GfxstreamEnd2EndVkTest,
                        ::testing::ValuesIn({
                            TestParams{
                                .with_gl = false,
                                .with_vk = true,
                            },
                        }),
                        &GetTestName);

}  // namespace
}  // namespace gfxstream