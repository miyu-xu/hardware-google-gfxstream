// Copyright (C) 2018 The Android Open Source Project
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

#if defined(__ANDROID__)
#include <hardware/hwvulkan.h>
#endif  // defined(__ANDROID__)

#include <log/log.h>

#include "gfxstream_vk_private.h"
#include "gfxstream_vk_entrypoints.h"

#include "vk_instance.h"
#include "vk_log.h"
#include "vk_alloc.h"

#include <errno.h>
#include <string.h>
#ifdef VK_USE_PLATFORM_FUCHSIA
#include <fidl/fuchsia.logger/cpp/wire.h>
#include <lib/syslog/global.h>
#include <lib/zx/channel.h>
#include <lib/zx/socket.h>
#include <lib/zxio/zxio.h>
#include <unistd.h>

#include "TraceProviderFuchsia.h"
#include "services/service_connector.h"
#endif

#include "HostConnection.h"
#include "ProcessPipe.h"
#include "ResourceTracker.h"
#include "VkEncoder.h"
#include "func_table.h"
#include "VirtGpu.h"

#if defined(__ANDROID__)

int OpenDevice(const hw_module_t* module, const char* id, hw_device_t** device);

hw_module_methods_t goldfish_vulkan_module_methods = {
    .open = OpenDevice
};

extern "C" __attribute__((visibility("default"))) hwvulkan_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = HWVULKAN_HARDWARE_MODULE_ID,
        .name = "Goldfish Vulkan Driver",
        .author = "The Android Open Source Project",
        .methods = &goldfish_vulkan_module_methods,
    },
};

int CloseDevice(struct hw_device_t* /*device*/) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::GetInstanceProcAddr");
    // nothing to do - opening a device doesn't allocate any resources
    return 0;
}

#endif // defined(__ANDROID__)

static HostConnection* getConnection(void) {
    auto hostCon = HostConnection::get();
    return hostCon;
}

static gfxstream::vk::VkEncoder* getVkEncoder(HostConnection* con) { return con->vkEncoder(); }

gfxstream::vk::ResourceTracker::ThreadingCallbacks threadingCallbacks = {
    .hostConnectionGetFunc = getConnection,
    .vkEncoderGetFunc = getVkEncoder,
};

VkResult SetupInstance(void) {
    uint32_t noRenderControlEnc = 0;
    HostConnection* hostCon = HostConnection::getOrCreate(kCapsetGfxStreamVulkan);
    if (!hostCon) {
        ALOGE("vulkan: Failed to get host connection\n");
        return VK_ERROR_DEVICE_LOST;
    }

    gfxstream::vk::ResourceTracker::get()->setupCaps(noRenderControlEnc);
    // Legacy goldfish path: could be deleted once goldfish not used guest-side.
    if (!noRenderControlEnc) {
        // Implicitly sets up sequence number
        ExtendedRCEncoderContext* rcEnc = hostCon->rcEncoder();
        if (!rcEnc) {
            ALOGE("vulkan: Failed to get renderControl encoder context\n");
            return VK_ERROR_DEVICE_LOST;
        }

        gfxstream::vk::ResourceTracker::get()->setupFeatures(rcEnc->featureInfo_const());
    }

    gfxstream::vk::ResourceTracker::get()->setThreadingCallbacks(threadingCallbacks);
    gfxstream::vk::ResourceTracker::get()->setSeqnoPtr(getSeqnoPtrForProcess());
    gfxstream::vk::VkEncoder* vkEnc = hostCon->vkEncoder();
    if (!vkEnc) {
        ALOGE("vulkan: Failed to get Vulkan encoder\n");
        return VK_ERROR_DEVICE_LOST;
    }

    return VK_SUCCESS;
}

#define VK_HOST_CONNECTION(ret)                                                    \
    HostConnection* hostCon = HostConnection::getOrCreate(kCapsetGfxStreamVulkan); \
    gfxstream::vk::VkEncoder* vkEnc = hostCon->vkEncoder();                        \
    if (!vkEnc) {                                                                  \
        ALOGE("vulkan: Failed to get Vulkan encoder\n");                           \
        return ret;                                                                \
    }

VKAPI_ATTR
VkResult EnumerateInstanceExtensionProperties(
    const char* layer_name,
    uint32_t* count,
    VkExtensionProperties* properties) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::EnumerateInstanceExtensionProperties");

    VkResult res = SetupInstance();
    if (res != VK_SUCCESS) {
        return res;
    }

    VK_HOST_CONNECTION(VK_ERROR_DEVICE_LOST)

    if (layer_name) {
        ALOGW(
            "Driver vkEnumerateInstanceExtensionProperties shouldn't be called "
            "with a layer name ('%s')",
            layer_name);
    }

    res = gfxstream::vk::ResourceTracker::get()->on_vkEnumerateInstanceExtensionProperties(
        vkEnc, VK_SUCCESS, layer_name, count, properties);

    return res;
}

VKAPI_ATTR
VkResult CreateInstance(const VkInstanceCreateInfo* create_info,
                        const VkAllocationCallbacks* allocator,
                        VkInstance* out_instance) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::CreateInstance");

    VkResult res = SetupInstance();
    if (res != VK_SUCCESS) {
        return res;
    }

    VK_HOST_CONNECTION(VK_ERROR_DEVICE_LOST)
    res = vkEnc->vkCreateInstance(create_info, nullptr, out_instance, true /* do lock */);

    return res;
}

static PFN_vkVoidFunction GetDeviceProcAddr(VkDevice device, const char* name) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::GetDeviceProcAddr");

    VK_HOST_CONNECTION(nullptr)

    if (!strcmp(name, "vkGetDeviceProcAddr")) {
        return (PFN_vkVoidFunction)(GetDeviceProcAddr);
    }

    return (PFN_vkVoidFunction)(nullptr);
}

VKAPI_ATTR
PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance, const char* name) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::GetInstanceProcAddr");

    VkResult res = SetupInstance();
    if (res != VK_SUCCESS) {
        return nullptr;
    }

    VK_HOST_CONNECTION(nullptr)

    if (!strcmp(name, "vkEnumerateInstanceExtensionProperties")) {
        return (PFN_vkVoidFunction)EnumerateInstanceExtensionProperties;
    }
    if (!strcmp(name, "vkCreateInstance")) {
        return (PFN_vkVoidFunction)CreateInstance;
    }
    if (!strcmp(name, "vkGetDeviceProcAddr")) {
        return (PFN_vkVoidFunction)(GetDeviceProcAddr);
    }

    return (PFN_vkVoidFunction)(nullptr);
}

#if defined(__ANDROID__)

hwvulkan_device_t goldfish_vulkan_device = {
    .common = {
        .tag = HARDWARE_DEVICE_TAG,
        .version = HWVULKAN_DEVICE_API_VERSION_0_1,
        .module = &HAL_MODULE_INFO_SYM.common,
        .close = CloseDevice,
    },
    .EnumerateInstanceExtensionProperties = EnumerateInstanceExtensionProperties,
    .CreateInstance = CreateInstance,
    .GetInstanceProcAddr = GetInstanceProcAddr,
};

int OpenDevice(const hw_module_t* /*module*/,
               const char* id,
               hw_device_t** device) {
    AEMU_SCOPED_TRACE("goldfish_vulkan::OpenDevice");

    if (strcmp(id, HWVULKAN_DEVICE_0) == 0) {
        *device = &goldfish_vulkan_device.common;
        gfxstream::vk::ResourceTracker::get();
        return 0;
    }
    return -ENOENT;
}

#elif VK_USE_PLATFORM_FUCHSIA

class VulkanDevice {
public:
    VulkanDevice() : mHostSupportsGoldfish(IsAccessible(QEMU_PIPE_PATH)) {
        InitLogger();
        InitTraceProvider();
        gfxstream::vk::ResourceTracker::get();
    }

    static void InitLogger();

    static bool IsAccessible(const char* name) {
        zx_handle_t handle = GetConnectToServiceFunction()(name);
        if (handle == ZX_HANDLE_INVALID)
            return false;

        zxio_storage_t io_storage;
        zx_status_t status = zxio_create(handle, &io_storage);
        if (status != ZX_OK)
            return false;

        status = zxio_close(&io_storage.io, /*should_wait=*/true);
        if (status != ZX_OK)
            return false;

        return true;
    }

    static VulkanDevice& GetInstance() {
        static VulkanDevice g_instance;
        return g_instance;
    }

    PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance, const char* name) {
        return ::GetInstanceProcAddr(instance, name);
    }

private:
    void InitTraceProvider();

    TraceProviderFuchsia mTraceProvider;
    const bool mHostSupportsGoldfish;
};

void VulkanDevice::InitLogger() {
  auto log_socket = ([] () -> std::optional<zx::socket> {
    fidl::ClientEnd<fuchsia_logger::LogSink> channel{zx::channel{
      GetConnectToServiceFunction()("/svc/fuchsia.logger.LogSink")}};
    if (!channel.is_valid())
      return std::nullopt;

    zx::socket local_socket, remote_socket;
    zx_status_t status = zx::socket::create(ZX_SOCKET_DATAGRAM, &local_socket, &remote_socket);
    if (status != ZX_OK)
      return std::nullopt;

    auto result = fidl::WireCall(channel)->Connect(std::move(remote_socket));

    if (!result.ok())
      return std::nullopt;

    return local_socket;
  })();
  if (!log_socket)
    return;

  fx_logger_config_t config = {
      .min_severity = FX_LOG_INFO,
      .log_sink_socket = log_socket->release(),
      .tags = nullptr,
      .num_tags = 0,
  };

  fx_log_reconfigure(&config);
}

void VulkanDevice::InitTraceProvider() {
    if (!mTraceProvider.Initialize()) {
        ALOGE("Trace provider failed to initialize");
    }
}

typedef VkResult(VKAPI_PTR *PFN_vkOpenInNamespaceAddr)(const char *pName, uint32_t handle);

namespace {

PFN_vkOpenInNamespaceAddr g_vulkan_connector;

zx_handle_t LocalConnectToServiceFunction(const char* pName) {
    zx::channel remote_endpoint, local_endpoint;
    zx_status_t status;
    if ((status = zx::channel::create(0, &remote_endpoint, &local_endpoint)) != ZX_OK) {
        ALOGE("zx::channel::create failed: %d", status);
        return ZX_HANDLE_INVALID;
    }
    if ((status = g_vulkan_connector(pName, remote_endpoint.release())) != ZX_OK) {
        ALOGE("vulkan_connector failed: %d", status);
        return ZX_HANDLE_INVALID;
    }
    return local_endpoint.release();
}

}

extern "C" __attribute__((visibility("default"))) void
vk_icdInitializeOpenInNamespaceCallback(PFN_vkOpenInNamespaceAddr callback) {
    g_vulkan_connector = callback;
    SetConnectToServiceFunction(&LocalConnectToServiceFunction);
}

#else
class VulkanDevice {
public:
    VulkanDevice() {
        gfxstream::vk::ResourceTracker::get();
    }

    static VulkanDevice& GetInstance() {
        static VulkanDevice g_instance;
        return g_instance;
    }

    PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance, const char* name) {
        return ::GetInstanceProcAddr(instance, name);
    }
};

static void get_instance_extensions(struct vk_instance_extension_table *instanceExts) {
    VkResult result = (VkResult)0;
    auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
    auto resources = gfxstream::vk::ResourceTracker::get();
    uint32_t numInstanceExts = 0;
    result = resources->on_vkEnumerateInstanceExtensionProperties(
                vkEnc, VK_SUCCESS, NULL, &numInstanceExts, NULL);
    if (VK_SUCCESS == result) {
        std::vector<VkExtensionProperties> extProps(numInstanceExts);
        result = resources->on_vkEnumerateInstanceExtensionProperties(
                    vkEnc, VK_SUCCESS, NULL, &numInstanceExts, extProps.data());
        if (VK_SUCCESS == result) {
            for (uint32_t i = 0; i < numInstanceExts; i++) {
                for (uint32_t j = 0; j < VK_INSTANCE_EXTENSION_COUNT; j++) {
                    if ((extProps[i].specVersion == vk_instance_extensions[j].specVersion)
                            && (0 == strncmp(extProps[i].extensionName, vk_instance_extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE))) {
                        instanceExts->extensions[j] = true;
                        break;
                    }
                }
            }
        }
    }
}

VkResult
gfxstream_vk_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                            const VkAllocationCallbacks *pAllocator,
                            VkInstance *pInstance)
{
    AEMU_SCOPED_TRACE("vkCreateInstance");

    struct gfxstream_vk_instance *instance;
    VkResult result;

    pAllocator = pAllocator ?: vk_default_allocator();
    instance = (struct gfxstream_vk_instance*)vk_zalloc(pAllocator, sizeof(*instance), 8,
                            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!instance)
        return vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);

    VkResult res = SetupInstance();
    if (res != VK_SUCCESS) {
        return res;
    }

    VK_HOST_CONNECTION(VK_ERROR_DEVICE_LOST);
    result = vkEnc->vkCreateInstance(pCreateInfo, nullptr, &instance->internal_object, true /* do lock */);

    struct vk_instance_dispatch_table dispatch_table;
    memset(&dispatch_table, 0, sizeof(struct vk_instance_dispatch_table));
    vk_instance_dispatch_table_from_entrypoints(
        &dispatch_table, &gfxstream_vk_instance_entrypoints, false);

    struct vk_instance_extension_table supported_extensions;
    get_instance_extensions(&supported_extensions);

    result = vk_instance_init(&instance->vk, &supported_extensions,
                                &dispatch_table, pCreateInfo, pAllocator);

    if (result != VK_SUCCESS) {
        vk_free(pAllocator, instance);
        return vk_error(NULL, result);
    }

    *pInstance = gfxstream_vk_instance_to_handle(instance);
    return VK_SUCCESS;
}

void
gfxstream_vk_DestroyInstance(VkInstance _instance,
                             const VkAllocationCallbacks *pAllocator)
{
    AEMU_SCOPED_TRACE("vkDestroyInstance");
    VK_FROM_HANDLE(gfxstream_vk_instance, instance, _instance);

    if (!instance)
        return;

    VK_HOST_CONNECTION()
    vkEnc->vkDestroyInstance(instance->internal_object, pAllocator, true /* do lock */);

    vk_instance_finish(&instance->vk);
    vk_free(&instance->vk.alloc, instance);
}

VkResult
gfxstream_vk_EnumerateInstanceExtensionProperties(const char* pLayerName,
                                                           uint32_t* pPropertyCount,
                                                           VkExtensionProperties* pProperties) {
    AEMU_SCOPED_TRACE("vkvkEnumerateInstanceExtensionProperties");

    VkResult res = SetupInstance();
    if (res != VK_SUCCESS) {
        return res;
    }

    VK_HOST_CONNECTION(VK_ERROR_DEVICE_LOST)

    VkResult vkEnumerateInstanceExtensionProperties_VkResult_return = (VkResult)0;
    auto resources = gfxstream::vk::ResourceTracker::get();
    vkEnumerateInstanceExtensionProperties_VkResult_return =
        resources->on_vkEnumerateInstanceExtensionProperties(vkEnc, VK_SUCCESS, pLayerName,
                                                                pPropertyCount, pProperties);
    return vkEnumerateInstanceExtensionProperties_VkResult_return;
}

/* The loader wants us to expose a second GetInstanceProcAddr function
 * to work around certain LD_PRELOAD issues seen in apps.
 */
PUBLIC
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName);

PUBLIC
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName)
{
    return gfxstream_vk_GetInstanceProcAddr(instance, pName);
}

/* vk_icd.h does not declare this function, so we declare it here to
 * suppress Wmissing-prototypes.
 */
PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion);

PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion)
{
    *pSupportedVersion = std::min(*pSupportedVersion, 3u);
    return VK_SUCCESS;
}


/*
 * CommandPool/CommandBuffer management
 */

VkResult gfxstream_vk_CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkCommandPool* pCommandPool) {
    AEMU_SCOPED_TRACE("vkCreateCommandPool");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VkResult result = (VkResult)0;
    struct gfxstream_vk_command_pool* gfxstream_pCommandPool = (gfxstream_vk_command_pool*)vk_zalloc2(
        &gfxstream_device->vk.alloc,
        pAllocator,
        sizeof(gfxstream_vk_command_pool),
        8,
        VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    result = gfxstream_pCommandPool ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == result) {
        result = vk_command_pool_init(&gfxstream_device->vk, &gfxstream_pCommandPool->vk, pCreateInfo, pAllocator);
    }
    if (VK_SUCCESS == result) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        result = vkEnc->vkCreateCommandPool(
            gfxstream_device->internal_object, pCreateInfo, pAllocator,
            &gfxstream_pCommandPool->internal_object, true /* do lock */);
    }
    *pCommandPool = gfxstream_vk_command_pool_to_handle(gfxstream_pCommandPool);
    return result;
}

void gfxstream_vk_DestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                                     const VkAllocationCallbacks* pAllocator) {
    AEMU_SCOPED_TRACE("vkDestroyCommandPool");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_command_pool, gfxstream_commandPool, commandPool);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        vkEnc->vkDestroyCommandPool(gfxstream_device->internal_object,
                                    gfxstream_commandPool->internal_object, pAllocator,
                                    true /* do lock */);
    }
    vk_command_pool_finish(&gfxstream_commandPool->vk);
    vk_free(&gfxstream_commandPool->vk.alloc, gfxstream_commandPool);
}

static VkResult vk_command_buffer_createOp(struct vk_command_pool *, struct vk_command_buffer **);
static void vk_command_buffer_resetOp(struct vk_command_buffer *, VkCommandBufferResetFlags);
static void vk_command_buffer_destroyOp(struct vk_command_buffer *);

static vk_command_buffer_ops gfxstream_vk_commandBufferOps = {
    .create = vk_command_buffer_createOp,
    .reset = vk_command_buffer_resetOp,
    .destroy = vk_command_buffer_destroyOp
};

VkResult vk_command_buffer_createOp(struct vk_command_pool *commandPool, struct vk_command_buffer **pCommandBuffer) {
    VkResult result = VK_SUCCESS;
    struct gfxstream_vk_command_buffer *gfxstream_commandBuffer = (struct gfxstream_vk_command_buffer*)vk_zalloc(
            &commandPool->alloc,
            sizeof(struct gfxstream_vk_command_buffer),
            8,
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (gfxstream_commandBuffer) {
        result = vk_command_buffer_init(commandPool, &gfxstream_commandBuffer->vk, &gfxstream_vk_commandBufferOps, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (VK_SUCCESS == result) {
            *pCommandBuffer = &gfxstream_commandBuffer->vk;
        }
    } else {
        result = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    return result;
}

void vk_command_buffer_resetOp(struct vk_command_buffer *commandBuffer, VkCommandBufferResetFlags flags) {
    (void)flags;
    vk_command_buffer_reset(commandBuffer);
}

void vk_command_buffer_destroyOp(struct vk_command_buffer *commandBuffer) {
    vk_command_buffer_finish(commandBuffer);
    vk_free(&commandBuffer->pool->alloc, commandBuffer);
}

VkResult gfxstream_vk_AllocateCommandBuffers(VkDevice device,
                                             const VkCommandBufferAllocateInfo* pAllocateInfo,
                                             VkCommandBuffer* pCommandBuffers) {
    AEMU_SCOPED_TRACE("vkAllocateCommandBuffers");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_command_pool, gfxstream_commandPool, pAllocateInfo->commandPool);
    VkResult result = (VkResult)0;
    std::vector<gfxstream_vk_command_buffer*> gfxstream_commandBuffers(pAllocateInfo->commandBufferCount);
    for(uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        result = vk_command_buffer_createOp(&gfxstream_commandPool->vk,  (vk_command_buffer**)&gfxstream_commandBuffers[i]);
        if (VK_SUCCESS == result) {
            gfxstream_commandBuffers[i]->vk.level = pAllocateInfo->level;
        } else {
            break;
        }
    }
    if (VK_SUCCESS == result) {
        // Create gfxstream-internal commandBuffer array
        std::vector<VkCommandBuffer> internal_objects(pAllocateInfo->commandBufferCount);
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        VkCommandBufferAllocateInfo internal_allocateInfo;
        internal_allocateInfo = *pAllocateInfo;
        internal_allocateInfo.commandPool = gfxstream_commandPool->internal_object;
        result = resources->on_vkAllocateCommandBuffers(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, &internal_allocateInfo,
            internal_objects.data());
        if (result == VK_SUCCESS) {
            gfxstream::vk::ResourceTracker::get()->addToCommandPool(
                gfxstream_commandPool->internal_object, pAllocateInfo->commandBufferCount, internal_objects.data());
            for (uint32_t i = 0; i < (uint32_t)internal_objects.size(); i++) {
                gfxstream_commandBuffers[i]->internal_object = internal_objects[i];
                // TODO: Also vk_command_buffer_init() on every mesa command buffer?
                pCommandBuffers[i] = gfxstream_vk_command_buffer_to_handle(gfxstream_commandBuffers[i]);
            }
        }
    }
    return result;
}

void gfxstream_vk_FreeCommandBuffers(VkDevice device, VkCommandPool commandPool,
                                     uint32_t commandBufferCount,
                                     const VkCommandBuffer* pCommandBuffers) {
    AEMU_SCOPED_TRACE("vkFreeCommandBuffers");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_command_pool, gfxstream_commandPool, commandPool);
    {
        // Set up internal commandBuffer array for gfxstream-internal call
        std::vector<VkCommandBuffer> internal_objects(commandBufferCount);
        for (uint32_t i = 0; i < commandBufferCount; i++) {
            VK_FROM_HANDLE(gfxstream_vk_command_buffer, gfxstream_commandBuffer, pCommandBuffers[i]);
            internal_objects[i] = gfxstream_commandBuffer->internal_object;
        }
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        vkEnc->vkFreeCommandBuffers(gfxstream_device->internal_object,
                                    gfxstream_commandPool->internal_object, commandBufferCount,
                                    internal_objects.data(), true /* do lock */);
    }
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        VK_FROM_HANDLE(gfxstream_vk_command_buffer, gfxstream_commandBuffer, pCommandBuffers[i]);
        vk_command_buffer_destroyOp(&gfxstream_commandBuffer->vk);
    }
}

VkResult gfxstream_vk_AllocateDescriptorSets(VkDevice device,
                                             const VkDescriptorSetAllocateInfo* pAllocateInfo,
                                             VkDescriptorSet* pDescriptorSets) {
    AEMU_SCOPED_TRACE("vkAllocateDescriptorSets");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VkResult result = (VkResult)0;
    std::vector<gfxstream_vk_descriptor_set*> gfxstream_descriptorSets(pAllocateInfo->descriptorSetCount);
    for(uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        gfxstream_descriptorSets[i] = (struct gfxstream_vk_descriptor_set*)vk_object_zalloc(
            &gfxstream_device->vk,
            NULL,
            sizeof(struct gfxstream_vk_descriptor_set),
            VK_OBJECT_TYPE_DESCRIPTOR_SET);
        result = gfxstream_descriptorSets[i] ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
        if (VK_SUCCESS != result) {
            break;
        }
    }
    if (VK_SUCCESS == result) {
        // Create gfxstream-internal descriptorSet array
        VkDescriptorSetAllocateInfo internal_allocateInfo = *pAllocateInfo;
        std::vector<VkDescriptorSetLayout> internal_VkDescriptorSetAllocateInfo_pSetLayouts(internal_allocateInfo.descriptorSetCount);
        {
            internal_allocateInfo = *pAllocateInfo;
            /* VkDescriptorSetAllocateInfo::descriptorPool */
            VK_FROM_HANDLE(gfxstream_vk_descriptor_pool, gfxstream_descriptorPool, internal_allocateInfo.descriptorPool);
            internal_allocateInfo.descriptorPool = gfxstream_descriptorPool->internal_object;
            /* VkDescriptorSetAllocateInfo::pSetLayouts */
            memset(internal_VkDescriptorSetAllocateInfo_pSetLayouts.data(), 0, sizeof(VkDescriptorSetLayout) * internal_allocateInfo.descriptorSetCount);
            for (uint32_t j = 0; j < internal_allocateInfo.descriptorSetCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_descriptor_set_layout, gfxstream_pSetLayouts, internal_allocateInfo.pSetLayouts[j]);
                internal_VkDescriptorSetAllocateInfo_pSetLayouts[j] = gfxstream_pSetLayouts->internal_object;
            }
            internal_allocateInfo.pSetLayouts = internal_VkDescriptorSetAllocateInfo_pSetLayouts.data();
        }
        std::vector<VkDescriptorSet> internal_objects(pAllocateInfo->descriptorSetCount);
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        result = resources->on_vkAllocateDescriptorSets(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, &internal_allocateInfo,
            internal_objects.data());
        if (VK_SUCCESS == result) {
            for (uint32_t i = 0; i < (uint32_t)internal_objects.size(); i++) {
                gfxstream_descriptorSets[i]->internal_object = internal_objects[i];
                pDescriptorSets[i] = gfxstream_vk_descriptor_set_to_handle(gfxstream_descriptorSets[i]);
            }
        }
    }
    return result;
}

VkResult gfxstream_vk_FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool,
                                         uint32_t descriptorSetCount,
                                         const VkDescriptorSet* pDescriptorSets) {
    AEMU_SCOPED_TRACE("vkFreeDescriptorSets");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_descriptor_pool, gfxstream_descriptorPool, descriptorPool);
    VkResult result = (VkResult)0;
    {
        // Set up internal desciptorSet array for gfxstream-internal call
        std::vector<VkDescriptorSet> internal_objects(descriptorSetCount);
        for (uint32_t i = 0; i < descriptorSetCount; i++) {
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_descriptorSet, pDescriptorSets[i]);
            internal_objects[i] = gfxstream_descriptorSet->internal_object;
        }
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        result = resources->on_vkFreeDescriptorSets(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object,
            gfxstream_descriptorPool->internal_object, descriptorSetCount,
            internal_objects.data());
    }
    if (VK_SUCCESS == result) {
        for (uint32_t i = 0; i < descriptorSetCount; i++) {
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_descriptorSet, pDescriptorSets[i]);
            vk_object_free(&gfxstream_device->vk, NULL, gfxstream_descriptorSet);
        }
    }
    return result;
}

#endif
