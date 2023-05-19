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
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// clang-format off
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "OpenGLESDispatch/gldefs.h"
#include "OpenGLESDispatch/gles_functions.h"
#include "OpenGLESDispatch/RenderEGL_functions.h"
#include "OpenGLESDispatch/RenderEGL_extensions_functions.h"

#define VULKAN_HPP_NAMESPACE vkhpp
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
// clang-format on

#include <android-base/expected.h>

#include "HostConnection.h"
#include "VirtGpu.h"
#include "drm_fourcc.h"
#include "render-utils/virtio-gpu-gfxstream-renderer.h"

namespace gfxstream {
namespace tests {

MATCHER(IsOk, "an ok result") {
  auto& result = arg;
  if (!result.ok()) {
    *result_listener << "which is an error with message: \""
                     << result.error()
                     << "\"";
    return false;
  }
  return true;
}

MATCHER(IsError, "an error result") {
  auto& result = arg;
  if (result.ok()) {
    *result_listener << "which is an ok result";
    return false;
  }
  return true;
}

template <typename GlType>
using GlExpected = android::base::expected<GlType, std::string>;

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
#define VK_ASSERT(x)                                    \
  ({                                                    \
    auto vk_expect_android_base_expected = (x);         \
    if (!vk_expect_android_base_expected.ok()) {        \
      ASSERT_THAT(vk_expect_android_base_expected.ok(), ::testing::IsTrue()); \
    };                                                  \
    std::move(vk_expect_android_base_expected.value()); \
  })

#define VK_EXPECT(x)                                    \
  ({                                                    \
    auto vk_expect_android_base_expected = (x);         \
    if (!vk_expect_android_base_expected.ok()) {        \
      return android::base::unexpected(                 \
        vk_expect_android_base_expected.error());       \
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

template <typename VkType>
using VkExpected = android::base::expected<VkType, vkhpp::Result>;

std::optional<uint32_t> DrmFormatToVirglFormat(uint32_t drmFormat);
std::optional<uint32_t> GlFormatToDrmFormat(uint32_t glFormat);

class TestingVirtGpuDevice;

class TestingVirtGpuBlobMapping : public VirtGpuBlobMapping {
  public:
    TestingVirtGpuBlobMapping(VirtGpuBlobPtr blob, uint8_t* mapped);
    ~TestingVirtGpuBlobMapping();

    uint8_t* asRawPtr(void) override;

  private:
    VirtGpuBlobPtr mBlob;
    uint8_t* mMapped = nullptr;
};

class TestingVirtGpuResource : public std::enable_shared_from_this<TestingVirtGpuResource>, public VirtGpuBlob {
  public:
    static std::shared_ptr<TestingVirtGpuResource> createBlob(
        uint32_t resourceId,
        std::shared_ptr<TestingVirtGpuDevice> device,
        std::shared_future<void> createCompleted,
        std::unique_ptr<std::optional<uint8_t*>> resourceMappedBytes);

    static std::shared_ptr<TestingVirtGpuResource> createPipe(
        uint32_t resourceId,
        std::shared_ptr<TestingVirtGpuDevice> device,
        std::shared_future<void> createCompleted,
        std::unique_ptr<uint8_t[]> resourceBytes);

    ~TestingVirtGpuResource();

    VirtGpuBlobMappingPtr createMapping(void) override;

    uint32_t getResourceHandle() override;
    uint32_t getBlobHandle() override;

    int exportBlob(VirtGpuExternalHandle& handle) override;
    int wait() override;

    int transferFromHost(uint32_t offset, uint32_t size) override;
    int transferToHost(uint32_t offset, uint32_t size) override;

  private:
    enum class ResourceType {
        kBlob,
        kPipe,
    };

    TestingVirtGpuResource(
        uint32_t resourceId,
        ResourceType resourceType,
        std::shared_ptr<TestingVirtGpuDevice> device,
        std::shared_future<void> createCompleted,
        std::unique_ptr<uint8_t[]> resourceGuestBytes = nullptr,
        std::unique_ptr<std::optional<uint8_t*>> resourceMappedBytes = nullptr);

    friend class TestingVirtGpuDevice;
    void addPendingCommandWaitable(std::shared_future<void> waitable);

    const uint32_t mResourceId;
    const ResourceType mResourceType;
    const std::shared_ptr<TestingVirtGpuDevice> mDevice;

    std::mutex mPendingCommandWaitablesMutex;
    std::vector<std::shared_future<void>> mPendingCommandWaitables;

    // For non-blob resources, the guest shadow memory.
    std::unique_ptr<uint8_t[]> mResourceGuestBytes;

    // For blob resources, the host memory once it is mapped.
    std::unique_ptr<std::optional<uint8_t*>> mResourceMappedHostBytes;
};

class TestingVirtGpuDevice : public std::enable_shared_from_this<TestingVirtGpuDevice>, public VirtGpuDevice {
  public:
    TestingVirtGpuDevice();
    ~TestingVirtGpuDevice();

    int64_t getDeviceHandle() override;

    VirtGpuCaps getCaps() override;

    VirtGpuBlobPtr createBlob(const struct VirtGpuCreateBlob& blobCreate) override;

    VirtGpuBlobPtr createPipeBlob(uint32_t size) override;

    VirtGpuBlobPtr createTexture(uint32_t width,
                                 uint32_t height,
                                 uint32_t drmFormat);

    int execBuffer(struct VirtGpuExecBuffer& execbuffer, VirtGpuBlobPtr blob);

    VirtGpuBlobPtr importBlob(const struct VirtGpuExternalHandle& handle);

  private:
    friend class TestingVirtGpuResource;

    std::shared_future<void> transferFromHost(uint32_t resourceId,
                                              uint32_t transferOffset,
                                              uint32_t transferSize);

    std::shared_future<void> transferToHost(uint32_t resourceId,
                                            uint32_t transferOffset,
                                            uint32_t transferSize);

  private:
    friend class TestingVirtGpuSyncHelper;

    int WaitOnEmulatedFence(int fenceAsFileDescriptor, int timeoutMilliseconds);

  public:
    // Public for callback from Gfxstream.
    void SignalEmulatedFence(uint32_t fenceId);

  private:
    uint32_t CreateEmulatedFence();

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
    struct VirtioGpuTaskMap {
        uint32_t resourceId;
        std::optional<uint8_t*>* resourceMappedHostBytes;
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
                                       VirtioGpuTaskMap,
                                       VirtioGpuTaskExecBuffer,
                                       VirtioGpuTaskTransferFromHost,
                                       VirtioGpuTaskTransferToHost>;
    struct VirtioGpuTaskWithWaitable {
        VirtioGpuTask task;
        std::promise<void> taskCompletedSignaler;
        std::optional<uint32_t> fence;
    };

    std::shared_future<void> EnqueueVirtioGpuTask(VirtioGpuTask task, std::optional<uint32_t> fence = std::nullopt);

    void DoTask(VirtioGpuTaskCreateBlob task);
    void DoTask(VirtioGpuTaskCreateResource task);
    void DoTask(VirtioGpuTaskMap task);
    void DoTask(VirtioGpuTaskExecBuffer task);
    void DoTask(VirtioGpuTaskTransferFromHost task);
    void DoTask(VirtioGpuTaskTransferToHost task);
    void DoTask(VirtioGpuTaskWithWaitable task);

    void RunVirtioGpuTaskProcessingLoop();

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

class TestingAHardwareBuffer {
  public:
    TestingAHardwareBuffer(uint32_t width,
                           uint32_t height,
                           std::shared_ptr<TestingVirtGpuResource> resource);

    uint32_t getResourceId() const;

    uint32_t getWidth() const;

    uint32_t getHeight() const;

    int getAndroidFormat() const;

    uint32_t getDrmFormat() const;

    AHardwareBuffer* asAHardwareBuffer();

    EGLClientBuffer asEglClientBuffer();

  private:
    uint32_t mWidth;
    uint32_t mHeight;
    std::shared_ptr<TestingVirtGpuResource> mResource;
};

class TestingVirtGpuGralloc : public Gralloc {
   public:
    TestingVirtGpuGralloc(std::shared_ptr<TestingVirtGpuDevice> device);

    uint32_t createColorBuffer(renderControl_client_context_t*,
                               int width,
                               int height,
                               uint32_t glFormat) override;

    int allocate(uint32_t width,
                 uint32_t height,
                 uint32_t format,
                 uint64_t usage,
                 AHardwareBuffer** outputAhb) override;

    std::unique_ptr<TestingAHardwareBuffer> allocate(uint32_t width,
                                                     uint32_t height,
                                                     uint32_t format);

    void acquire(AHardwareBuffer* ahb) override;
    void release(AHardwareBuffer* ahb) override;

    uint32_t getHostHandle(const native_handle_t* handle) override;
    uint32_t getHostHandle(const AHardwareBuffer* handle) override;

    int getFormat(const native_handle_t* handle) override;
    int getFormat(const AHardwareBuffer* handle) override;

    uint32_t getFormatDrmFourcc(const AHardwareBuffer* handle) override;

    size_t getAllocatedSize(const native_handle_t*) override;
    size_t getAllocatedSize(const AHardwareBuffer*) override;

  private:
    std::unordered_map<uint32_t, std::unique_ptr<TestingAHardwareBuffer>> mAllocatedColorBuffers;
    std::shared_ptr<TestingVirtGpuDevice> mDevice;
};

class TestingANativeWindow {
  public:
    TestingANativeWindow(uint32_t width,
                         uint32_t height,
                         uint32_t format,
                         std::vector<std::unique_ptr<TestingAHardwareBuffer>> buffers);

    EGLNativeWindowType asEglNativeWindowType();

    uint32_t getWidth() const;

    uint32_t getHeight() const;

    int getFormat() const;

    int queueBuffer(EGLClientBuffer buffer, int fence);
    int dequeueBuffer(EGLClientBuffer* buffer, int* fence);
    int cancelBuffer(EGLClientBuffer buffer);

  private:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
    std::vector<std::unique_ptr<TestingAHardwareBuffer>> mBuffers;

    struct QueuedAHB {
        TestingAHardwareBuffer* ahb;
        int fence = -1;
    };
    std::deque<QueuedAHB> mBufferQueue;
};

class TestingVirtGpuANativeWindowHelper : public ANativeWindowHelper {
  public:
    bool isValid(EGLNativeWindowType window) override;
    bool isValid(EGLClientBuffer buffer) override;

    void acquire(EGLNativeWindowType window) override;
    void release(EGLNativeWindowType window) override;

    void acquire(EGLClientBuffer buffer) override;
    void release(EGLClientBuffer buffer) override;

    int getConsumerUsage(EGLNativeWindowType window, int* usage) override;
    void setUsage(EGLNativeWindowType window, int usage) override;

    int getWidth(EGLNativeWindowType window) override;
    int getHeight(EGLNativeWindowType window) override;

    int getWidth(EGLClientBuffer buffer) override;
    int getHeight(EGLClientBuffer buffer) override;

    int getFormat(EGLClientBuffer buffer, Gralloc* helper) override;

    void setSwapInterval(EGLNativeWindowType window, int interval) override;

    int queueBuffer(EGLNativeWindowType window, EGLClientBuffer buffer, int fence) override;
    int dequeueBuffer(EGLNativeWindowType window, EGLClientBuffer* buffer, int* fence) override;
    int cancelBuffer(EGLNativeWindowType window, EGLClientBuffer buffer) override;

    int getHostHandle(EGLClientBuffer buffer, Gralloc*) override;
};

class TestingVirtGpuSyncHelper : public SyncHelper {
  public:
    TestingVirtGpuSyncHelper(std::shared_ptr<TestingVirtGpuDevice> device);

    int wait(int syncFd, int timeoutMilliseconds) override;

    int dup(int syncFd) override;

    int close(int) override;

  private:
    std::shared_ptr<TestingVirtGpuDevice> mDevice;
};

struct TestParams {
    bool with_gl;
    bool with_vk;

    std::string ToString() const;
    friend std::ostream& operator<<(std::ostream& os, const TestParams& params);
};

std::string GetTestName(const ::testing::TestParamInfo<TestParams>& info);

class GfxstreamEnd2EndTest : public ::testing::TestWithParam<TestParams> {
  protected:
    struct GuestGlDispatchTable {
        #define DECLARE_EGL_FUNCTION(return_type, function_name, signature) \
            return_type (*function_name) signature = nullptr;

        #define DECLARE_GLES_FUNCTION(return_type, function_name, signature, args) \
            return_type (*function_name) signature = nullptr;

        LIST_RENDER_EGL_FUNCTIONS(DECLARE_EGL_FUNCTION)
        LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(DECLARE_EGL_FUNCTION)
        LIST_GLES_FUNCTIONS(DECLARE_GLES_FUNCTION, DECLARE_GLES_FUNCTION)
    };

    std::unique_ptr<GuestGlDispatchTable> SetupGuestGl();

    std::unique_ptr<vkhpp::raii::Context> SetupGuestVk();

    void SetUp() override;

    void TearDownGuest();
    void TearDownHost();
    void TearDown();

    std::unique_ptr<TestingANativeWindow> CreateEmulatedANW(uint32_t width, uint32_t height);

    void SetUpEglContextAndSurface(uint32_t contextVersion,
                                   uint32_t width,
                                   uint32_t height,
                                   EGLDisplay* outDisplay,
                                   EGLContext* outContext,
                                   EGLSurface* outSurface);

    void TearDownEglContextAndSurface(EGLDisplay display,
                                      EGLContext context,
                                      EGLSurface surface);

    GlExpected<GLuint> SetUpShader(GLenum type, const std::string& source);

    GlExpected<GLuint> SetUpProgram(const std::string& vertSource,
                                    const std::string& fragSource);

    struct TypicalVkTestEnvironment {
        vkhpp::raii::Instance instance;
        vkhpp::raii::PhysicalDevice physicalDevice;
        vkhpp::raii::Device device;
        vkhpp::raii::Queue queue;
        uint32_t queueFamilyIndex;
    };
    VkExpected<TypicalVkTestEnvironment> SetUpTypicalVkTestEnvironment();

    std::shared_ptr<TestingVirtGpuDevice> mDevice;
    std::unique_ptr<TestingVirtGpuANativeWindowHelper> mAnwHelper;
    std::unique_ptr<TestingVirtGpuGralloc> mGralloc;
    std::unique_ptr<TestingVirtGpuSyncHelper> mSync;
    std::unique_ptr<GuestGlDispatchTable> mGl;
    std::unique_ptr<vkhpp::raii::Context> mVk;
};

}  // namespace tests
}  // namespace gfxstream