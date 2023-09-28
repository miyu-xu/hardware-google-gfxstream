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

static const struct vk_instance_extension_table gfxstream_vk_instance_extensions = {
   .KHR_external_fence_capabilities = true,
   .KHR_external_memory_capabilities = true,
   .KHR_external_semaphore_capabilities = true,
   .KHR_get_physical_device_properties2 = true,
   .EXT_debug_report = true,
   .EXT_debug_utils = true,
};

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

   memset(&instance->dispatch_table, 0, sizeof(struct vk_instance_dispatch_table));
   vk_instance_dispatch_table_from_entrypoints(
      &instance->dispatch_table, &gfxstream_vk_instance_entrypoints, false);

   result = vk_instance_init(&instance->vk, &gfxstream_vk_instance_extensions,
                             &instance->dispatch_table, pCreateInfo, pAllocator);

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

PFN_vkVoidFunction
gfxstream_vk_GetInstanceProcAddr(VkInstance _instance, const char *pName)
{
   VK_FROM_HANDLE(gfxstream_vk_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk, &gfxstream_vk_instance_entrypoints,
                                    pName);
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

/* With version 4+ of the loader interface the ICD should expose
 * vk_icdGetPhysicalDeviceProcAddr()
 */
PUBLIC
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName);

PFN_vkVoidFunction
vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName)
{
   VK_FROM_HANDLE(gfxstream_vk_instance, instance, _instance);

   return vk_instance_get_physical_device_proc_addr(&instance->vk, pName);
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

PFN_vkVoidFunction
gfxstream_vk_GetDeviceProcAddr(VkDevice _device, const char *pName)
{
    AEMU_SCOPED_TRACE("vkGetDeviceProcAddr");
    VK_FROM_HANDLE(gfxstream_vk_device, device, _device);
    return vk_device_get_proc_addr(&device->vk, pName);
}

VkResult gfxstream_vk_EnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount,
                                               VkPhysicalDevice* pPhysicalDevices) {
    AEMU_SCOPED_TRACE("vkEnumeratePhysicalDevices");
    VK_FROM_HANDLE(gfxstream_vk_instance, gfxstream_instance, instance);
    VkResult result = (VkResult)0;
    std::vector<VkPhysicalDevice> internal_list;
    VkPhysicalDevice* internal_objects_pointer = NULL;
    if (pPhysicalDevices) {
        internal_list.reserve(*pPhysicalDeviceCount);
        internal_objects_pointer = internal_list.data();
    }
    auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
    auto resources = gfxstream::vk::ResourceTracker::get();
    result = resources->on_vkEnumeratePhysicalDevices(
        vkEnc, VK_SUCCESS, gfxstream_instance->internal_object, pPhysicalDeviceCount,
        internal_objects_pointer);

    if (VK_SUCCESS == result && pPhysicalDevices) {
        struct gfxstream_vk_physical_device* gfxstream_physicalDevices = (struct gfxstream_vk_physical_device*)vk_zalloc(
            &gfxstream_instance->vk.alloc,
            ((*pPhysicalDeviceCount) * sizeof(struct gfxstream_vk_physical_device)),
            8,
            VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
        result = gfxstream_physicalDevices ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;

        struct vk_physical_device_dispatch_table dispatch_table;
        memset(&dispatch_table, 0, sizeof(struct vk_physical_device_dispatch_table));
        vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table, &gfxstream_vk_physical_device_entrypoints, false);
        if (VK_SUCCESS == result) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                // Initialize the mesa object
                result = vk_physical_device_init(&gfxstream_physicalDevices[i].vk, &gfxstream_instance->vk, NULL, NULL, NULL, &dispatch_table);
                if (VK_SUCCESS != result) {
                    break;
                }
                // Set instance reference
                gfxstream_physicalDevices[i].instance = gfxstream_instance;
                // TODO: Add list of physicalDevice enumeration allocations to the instance object
                // Set the gfxstream-internal object
                gfxstream_physicalDevices[i].internal_object = internal_objects_pointer[i];
                pPhysicalDevices[i] = gfxstream_vk_physical_device_to_handle(&gfxstream_physicalDevices[i]);
            }
            if (VK_SUCCESS != result) {
                vk_free(&gfxstream_instance->vk.alloc, gfxstream_physicalDevices);
            }
        }
    }
    return result;
}

VkResult gfxstream_vk_EnumeratePhysicalDeviceGroups(
    VkInstance instance, uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
    AEMU_SCOPED_TRACE("vkEnumeratePhysicalDeviceGroups");
    VK_FROM_HANDLE(gfxstream_vk_instance, gfxstream_instance, instance);
    VkResult result = (VkResult)0;
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        result = vkEnc->vkEnumeratePhysicalDeviceGroups(
            gfxstream_instance->internal_object, pPhysicalDeviceGroupCount,
            pPhysicalDeviceGroupProperties, true /* do lock */);
    }
    if (pPhysicalDeviceGroupProperties) {
        for (uint32_t group = 0; group < *pPhysicalDeviceGroupCount; group++) {
            struct gfxstream_vk_physical_device* gfxstream_physicalDevices = (struct gfxstream_vk_physical_device*)vk_zalloc(
                &gfxstream_instance->vk.alloc,
                ((pPhysicalDeviceGroupProperties[group].physicalDeviceCount) * sizeof(struct gfxstream_vk_physical_device)),
                8,
                VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
            result = gfxstream_physicalDevices ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
            if (VK_SUCCESS == result) {
                struct vk_physical_device_dispatch_table dispatch_table;
                memset(&dispatch_table, 0, sizeof(struct vk_physical_device_dispatch_table));
                vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table, &gfxstream_vk_physical_device_entrypoints, false);
                for (uint32_t device = 0; device < pPhysicalDeviceGroupProperties[group].physicalDeviceCount; device++) {
                    // Initialize the mesa object
                    result = vk_physical_device_init(&gfxstream_physicalDevices[device].vk, &gfxstream_instance->vk, NULL, NULL, NULL, &dispatch_table);
                    if (VK_SUCCESS != result) {
                        break;
                    }
                    // Set instance reference
                    gfxstream_physicalDevices[device].instance = gfxstream_instance;
                    // TODO: Add list of physicalDevice enumeration allocations to the instance object
                    // Set the gfxstream-internal object
                    gfxstream_physicalDevices[device].internal_object = pPhysicalDeviceGroupProperties[group].physicalDevices[device];
                    // Set the output handle
                    pPhysicalDeviceGroupProperties[group].physicalDevices[device] = gfxstream_vk_physical_device_to_handle(&gfxstream_physicalDevices[device]);
                }
            }
            if (VK_SUCCESS != result) {
                break;
            }
        }
        // TODO: Clean-up for failed allocations/physical_device_init
    }

    return result;
}
VkResult gfxstream_vk_CreateDevice(VkPhysicalDevice physicalDevice,
                                   const VkDeviceCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    AEMU_SCOPED_TRACE("vkCreateDevice");
    VK_FROM_HANDLE(gfxstream_vk_physical_device, gfxstream_physicalDevice, physicalDevice);
    VkResult result = (VkResult)0;

    const VkAllocationCallbacks* pMesaAllocator = pAllocator ?: &gfxstream_physicalDevice->instance->vk.alloc;
    struct gfxstream_vk_device* gfxstream_device = (struct gfxstream_vk_device*)vk_zalloc(
        pMesaAllocator, sizeof(struct gfxstream_vk_device), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    result = gfxstream_device ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == result) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        result = vkEnc->vkCreateDevice(
            gfxstream_physicalDevice->internal_object, pCreateInfo, pAllocator,
            &gfxstream_device->internal_object, true /* do lock */);
    }
    if (VK_SUCCESS == result) {
        struct vk_device_dispatch_table dispatch_table;
        memset(&dispatch_table, 0, sizeof(struct vk_device_dispatch_table));
        vk_device_dispatch_table_from_entrypoints(&dispatch_table, &gfxstream_vk_device_entrypoints, false);

        result = vk_device_init(&gfxstream_device->vk, &gfxstream_physicalDevice->vk, &dispatch_table, pCreateInfo, pMesaAllocator);
    }
    if (VK_SUCCESS == result) {
        // TODO: wsi_device_init(&gfxstream_device->wsi_device, ...)
        gfxstream_device->physical_device = gfxstream_physicalDevice;
        // TODO: Initialize cmd_dispatch for emulated secondary command buffer support?
        gfxstream_device->vk.command_dispatch_table = &gfxstream_device->cmd_dispatch;
        *pDevice = gfxstream_vk_device_to_handle(gfxstream_device);
    }
    else {
        vk_free(pMesaAllocator, gfxstream_device);
    }

    return result;
}

void gfxstream_vk_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    AEMU_SCOPED_TRACE("vkDestroyDevice");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    if (!device)
        return;

    auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
    vkEnc->vkDestroyDevice(gfxstream_device->internal_object, pAllocator, true /* do lock */);

    vk_device_finish(&gfxstream_device->vk);
    vk_free(&gfxstream_device->vk.alloc, gfxstream_device);
}

void gfxstream_vk_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex,
                                 VkQueue* pQueue) {
    AEMU_SCOPED_TRACE("vkGetDeviceQueue");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    struct gfxstream_vk_queue* gfxstream_queue = (struct gfxstream_vk_queue*)vk_zalloc(
        &gfxstream_device->vk.alloc, sizeof(struct gfxstream_vk_queue), 8, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    VkResult result = gfxstream_queue ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == result) {
        VkDeviceQueueCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = NULL,
        };
        result = vk_queue_init(&gfxstream_queue->vk, &gfxstream_device->vk, &createInfo, queueIndex);
    }
    if (VK_SUCCESS == result) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        vkEnc->vkGetDeviceQueue(gfxstream_device->internal_object, queueFamilyIndex, queueIndex,
                                &gfxstream_queue->internal_object, true /* do lock */);

        gfxstream_queue->device = gfxstream_device;
        *pQueue = gfxstream_vk_queue_to_handle(gfxstream_queue);
    } else {
        *pQueue = VK_NULL_HANDLE;
    }
}

void gfxstream_vk_GetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2* pQueueInfo,
                                  VkQueue* pQueue) {
    AEMU_SCOPED_TRACE("vkGetDeviceQueue2");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    struct gfxstream_vk_queue* gfxstream_queue = (struct gfxstream_vk_queue*)vk_zalloc(
        &gfxstream_device->vk.alloc, sizeof(struct gfxstream_vk_queue), 8, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    VkResult result = gfxstream_queue ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == result) {
        VkDeviceQueueCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = pQueueInfo->flags,
            .queueFamilyIndex = pQueueInfo->queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = NULL,
        };
        result = vk_queue_init(&gfxstream_queue->vk, &gfxstream_device->vk, &createInfo, pQueueInfo->queueIndex);
    }
    if (VK_SUCCESS == result) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        vkEnc->vkGetDeviceQueue2(gfxstream_device->internal_object, pQueueInfo,
                                 &gfxstream_queue->internal_object, true /* do lock */);

        gfxstream_queue->device = gfxstream_device;
        *pQueue = gfxstream_vk_queue_to_handle(gfxstream_queue);
    } else {
        *pQueue = VK_NULL_HANDLE;
    }
}

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

VkResult gfxstream_vk_AllocateCommandBuffers(VkDevice device,
                                             const VkCommandBufferAllocateInfo* pAllocateInfo,
                                             VkCommandBuffer* pCommandBuffers) {
    AEMU_SCOPED_TRACE("vkAllocateCommandBuffers");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VkResult result = (VkResult)0;
    std::vector<gfxstream_vk_command_buffer*> gfxstream_commandBuffers(pAllocateInfo->commandBufferCount);
    for(uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        gfxstream_commandBuffers[i] = (struct gfxstream_vk_command_buffer*)vk_zalloc(
            &gfxstream_device->vk.alloc,
            sizeof(struct gfxstream_vk_command_buffer),
            8,
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
        result = gfxstream_commandBuffers[i] ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
        if (VK_SUCCESS != result) {
            break;
        }
    }
    for(uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        VK_FROM_HANDLE(gfxstream_vk_command_pool, gfxstream_commandPool, pAllocateInfo->commandPool);
        // TODO: Provide vk_command_buffers_ops ?
        result = vk_command_buffer_init(&gfxstream_commandPool->vk, &gfxstream_commandBuffers[i]->vk, NULL, pAllocateInfo->level);
        if (VK_SUCCESS != result) {
            break;
        }
    }
    if (VK_SUCCESS == result) {
        // Create gfxstream-internal commandBuffer array
        std::vector<VkCommandBuffer> internal_objects(pAllocateInfo->commandBufferCount);
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        result = resources->on_vkAllocateCommandBuffers(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, pAllocateInfo,
            internal_objects.data());
        if (result == VK_SUCCESS) {
            VK_FROM_HANDLE(gfxstream_vk_command_pool, gfxstream_commandPool, pAllocateInfo->commandPool);
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
        vk_command_buffer_finish(&gfxstream_commandBuffer->vk);
        vk_free(&gfxstream_device->vk.alloc, gfxstream_commandBuffer);
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
        std::vector<VkDescriptorSet> internal_objects(pAllocateInfo->descriptorSetCount);
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        result = resources->on_vkAllocateDescriptorSets(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, pAllocateInfo,
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

/*
 * Handle types in nested compoundTypes
 */

VkResult gfxstream_vk_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache,
                                              uint32_t createInfoCount,
                                              const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkPipeline* pPipelines) {
    AEMU_SCOPED_TRACE("vkCreateGraphicsPipelines");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_pipeline_cache, gfxstream_pipelineCache, pipelineCache);
    VkResult vkCreateGraphicsPipelines_VkResult_return = (VkResult)0;
    struct gfxstream_vk_pipeline* gfxstream_pPipelines = (struct gfxstream_vk_pipeline*)vk_object_zalloc(
        &gfxstream_device->vk,
        pAllocator,
        sizeof(struct gfxstream_vk_pipeline),
        VK_OBJECT_TYPE_PIPELINE);
    vkCreateGraphicsPipelines_VkResult_return =
        gfxstream_pPipelines ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == vkCreateGraphicsPipelines_VkResult_return) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        std::vector<VkGraphicsPipelineCreateInfo> internal_pCreateInfos(createInfoCount);
        std::vector<std::vector<VkPipelineShaderStageCreateInfo>> internal_nested_pStages(createInfoCount);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            internal_pCreateInfos[i] = pCreateInfos[i];
            /* VkGraphicsPipelineCreateInfo::pStages */
            internal_nested_pStages[i].reserve(internal_pCreateInfos[i].stageCount);
            for (uint32_t j = 0; j < internal_pCreateInfos[i].stageCount; ++j) {
                internal_nested_pStages[i][j] = internal_pCreateInfos[i].pStages[j];
                VK_FROM_HANDLE(gfxstream_vk_shader_module, gfxstream_module,
                               internal_nested_pStages[i][j].module);
                internal_nested_pStages[i][j].module = gfxstream_module->internal_object;
            }
            internal_pCreateInfos[i].pStages = internal_nested_pStages[i].data();
            /* VkGraphicsPipelineCreateInfo::layout */
            VK_FROM_HANDLE(gfxstream_vk_pipeline_layout, gfxstream_layout,
                           internal_pCreateInfos[i].layout);
            internal_pCreateInfos[i].layout = gfxstream_layout->internal_object;
            /* VkGraphicsPipelineCreateInfo::renderPass */
            VK_FROM_HANDLE(gfxstream_vk_render_pass, gfxstream_renderPass,
                           internal_pCreateInfos[i].renderPass);
            internal_pCreateInfos[i].renderPass = gfxstream_renderPass->internal_object;
            /* VkGraphicsPipelineCreateInfo::basePipelineHandle */
            VK_FROM_HANDLE(gfxstream_vk_pipeline, gfxstream_basePipelineHandle,
                           internal_pCreateInfos[i].basePipelineHandle);
            internal_pCreateInfos[i].basePipelineHandle =
                gfxstream_basePipelineHandle->internal_object;
        }
        auto resources = gfxstream::vk::ResourceTracker::get();
        vkCreateGraphicsPipelines_VkResult_return = resources->on_vkCreateGraphicsPipelines(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object,
            gfxstream_pipelineCache->internal_object, createInfoCount, internal_pCreateInfos.data(),
            pAllocator, &gfxstream_pPipelines->internal_object);
    }
    *pPipelines = gfxstream_vk_pipeline_to_handle(gfxstream_pPipelines);
    return vkCreateGraphicsPipelines_VkResult_return;
}
VkResult gfxstream_vk_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache,
                                             uint32_t createInfoCount,
                                             const VkComputePipelineCreateInfo* pCreateInfos,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkPipeline* pPipelines) {
    AEMU_SCOPED_TRACE("vkCreateComputePipelines");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VK_FROM_HANDLE(gfxstream_vk_pipeline_cache, gfxstream_pipelineCache, pipelineCache);
    VkResult vkCreateComputePipelines_VkResult_return = (VkResult)0;
    struct gfxstream_vk_pipeline* gfxstream_pPipelines = (struct gfxstream_vk_pipeline*)vk_object_zalloc(
        &gfxstream_device->vk,
        pAllocator,
        sizeof(struct gfxstream_vk_pipeline),
        VK_OBJECT_TYPE_PIPELINE);
    vkCreateComputePipelines_VkResult_return =
        gfxstream_pPipelines ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == vkCreateComputePipelines_VkResult_return) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        std::vector<VkComputePipelineCreateInfo> internal_pCreateInfos(createInfoCount);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            internal_pCreateInfos[i] = pCreateInfos[i];
            /* VkComputePipelineCreateInfo::stage */
            internal_pCreateInfos[i].stage = internal_pCreateInfos[i].stage;
            VK_FROM_HANDLE(gfxstream_vk_shader_module, gfxstream_module,
                           internal_pCreateInfos[i].stage.module);
            internal_pCreateInfos[i].stage.module = gfxstream_module->internal_object;
            /* VkComputePipelineCreateInfo::layout */
            VK_FROM_HANDLE(gfxstream_vk_pipeline_layout, gfxstream_layout,
                           internal_pCreateInfos[i].layout);
            internal_pCreateInfos[i].layout = gfxstream_layout->internal_object;
            /* VkComputePipelineCreateInfo::basePipelineHandle */
            VK_FROM_HANDLE(gfxstream_vk_pipeline, gfxstream_basePipelineHandle,
                           internal_pCreateInfos[i].basePipelineHandle);
            internal_pCreateInfos[i].basePipelineHandle =
                gfxstream_basePipelineHandle->internal_object;
        }
        vkCreateComputePipelines_VkResult_return = vkEnc->vkCreateComputePipelines(
            gfxstream_device->internal_object, gfxstream_pipelineCache->internal_object,
            createInfoCount, internal_pCreateInfos.data(), pAllocator,
            &gfxstream_pPipelines->internal_object, true /* do lock */);
    }
    *pPipelines = gfxstream_vk_pipeline_to_handle(gfxstream_pPipelines);
    return vkCreateComputePipelines_VkResult_return;
}

void gfxstream_vk_UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                       const VkWriteDescriptorSet* pDescriptorWrites,
                                       uint32_t descriptorCopyCount,
                                       const VkCopyDescriptorSet* pDescriptorCopies) {
    AEMU_SCOPED_TRACE("vkUpdateDescriptorSets");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        std::vector<VkWriteDescriptorSet> internal_pDescriptorWrites(descriptorWriteCount);
        std::vector<std::vector<VkDescriptorImageInfo>> internal_nested_pImageInfo(descriptorWriteCount);
        std::vector<std::vector<VkDescriptorBufferInfo>> internal_nested_pBufferInfo(descriptorWriteCount);
        std::vector<std::vector<VkBufferView>> internal_nested_pTexelBufferView(
            descriptorWriteCount);
        for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
            internal_pDescriptorWrites[i] = pDescriptorWrites[i];
            /* VkWriteDescriptorSet::dstSet */
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_dstSet,
                           internal_pDescriptorWrites[i].dstSet);
            internal_pDescriptorWrites[i].dstSet = gfxstream_dstSet->internal_object;
            /* VkWriteDescriptorSet::pImageInfo */
            internal_nested_pImageInfo[i].reserve(internal_pDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pDescriptorWrites[i].descriptorCount; ++j) {
                internal_nested_pImageInfo[i][j] = internal_pDescriptorWrites[i].pImageInfo[j];
                VK_FROM_HANDLE(gfxstream_vk_sampler, gfxstream_sampler, internal_nested_pImageInfo[i][j].sampler);
                internal_nested_pImageInfo[i][j].sampler = gfxstream_sampler->internal_object;
                VK_FROM_HANDLE(gfxstream_vk_image_view, gfxstream_imageView, internal_nested_pImageInfo[i][j].imageView);
                internal_nested_pImageInfo[i][j].imageView = gfxstream_imageView->internal_object;
            }
            internal_pDescriptorWrites[i].pImageInfo = internal_nested_pImageInfo[i].data();
            /* VkWriteDescriptorSet::pBufferInfo */
            internal_nested_pBufferInfo[i].reserve(internal_pDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pDescriptorWrites[i].descriptorCount; ++j) {
                internal_nested_pBufferInfo[i][j] = internal_pDescriptorWrites[i].pBufferInfo[j];
                VK_FROM_HANDLE(gfxstream_vk_buffer, gfxstream_buffer, internal_nested_pBufferInfo[i][j].buffer);
                internal_nested_pBufferInfo[i][j].buffer = gfxstream_buffer->internal_object;
            }
            internal_pDescriptorWrites[i].pBufferInfo = internal_nested_pBufferInfo[i].data();
            /* VkWriteDescriptorSet::pTexelBufferView */
            internal_nested_pTexelBufferView[i].reserve(
                internal_pDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pDescriptorWrites[i].descriptorCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_buffer_view, gfxstream_pTexelBufferView,
                               internal_pDescriptorWrites[i].pTexelBufferView[j]);
                internal_nested_pTexelBufferView[i][j] =
                    gfxstream_pTexelBufferView->internal_object;
            }
            internal_pDescriptorWrites[i].pTexelBufferView =
                internal_nested_pTexelBufferView[i].data();
        }
        std::vector<VkCopyDescriptorSet> internal_pDescriptorCopies(descriptorCopyCount);
        for (uint32_t i = 0; i < descriptorCopyCount; ++i) {
            internal_pDescriptorCopies[i] = pDescriptorCopies[i];
            /* VkCopyDescriptorSet::srcSet */
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_srcSet,
                           internal_pDescriptorCopies[i].srcSet);
            internal_pDescriptorCopies[i].srcSet = gfxstream_srcSet->internal_object;
            /* VkCopyDescriptorSet::dstSet */
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_dstSet,
                           internal_pDescriptorCopies[i].dstSet);
            internal_pDescriptorCopies[i].dstSet = gfxstream_dstSet->internal_object;
        }
        auto resources = gfxstream::vk::ResourceTracker::get();
        resources->on_vkUpdateDescriptorSets(vkEnc, gfxstream_device->internal_object,
                                             descriptorWriteCount,
                                             internal_pDescriptorWrites.data(), descriptorCopyCount,
                                             internal_pDescriptorCopies.data());
    }
}

void gfxstream_vk_QueueCommitDescriptorSetUpdatesGOOGLE(
    VkQueue queue, uint32_t descriptorPoolCount, const VkDescriptorPool* pDescriptorPools,
    uint32_t descriptorSetCount, const VkDescriptorSetLayout* pSetLayouts,
    const uint64_t* pDescriptorSetPoolIds, const uint32_t* pDescriptorSetWhichPool,
    const uint32_t* pDescriptorSetPendingAllocation,
    const uint32_t* pDescriptorWriteStartingIndices, uint32_t pendingDescriptorWriteCount,
    const VkWriteDescriptorSet* pPendingDescriptorWrites) {
    AEMU_SCOPED_TRACE("vkQueueCommitDescriptorSetUpdatesGOOGLE");
    VK_FROM_HANDLE(gfxstream_vk_queue, gfxstream_queue, queue);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getQueueEncoder(queue);
        std::vector<VkDescriptorPool> internal_pDescriptorPools(descriptorPoolCount);
        for (uint32_t i = 0; i < descriptorPoolCount; ++i) {
            VK_FROM_HANDLE(gfxstream_vk_descriptor_pool, gfxstream_pDescriptorPools,
                           pDescriptorPools[i]);
            internal_pDescriptorPools[i] = gfxstream_pDescriptorPools->internal_object;
        }
        std::vector<VkDescriptorSetLayout> internal_pSetLayouts(descriptorSetCount);
        for (uint32_t i = 0; i < descriptorSetCount; ++i) {
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set_layout, gfxstream_pSetLayouts,
                           pSetLayouts[i]);
            internal_pSetLayouts[i] = gfxstream_pSetLayouts->internal_object;
        }
        std::vector<VkWriteDescriptorSet> internal_pPendingDescriptorWrites(
            pendingDescriptorWriteCount);
        std::vector<std::vector<VkDescriptorImageInfo>> internal_nested_pImageInfo(pendingDescriptorWriteCount);
        std::vector<std::vector<VkDescriptorBufferInfo>> internal_nested_pBufferInfo(pendingDescriptorWriteCount);
        std::vector<std::vector<VkBufferView>> internal_nested_pTexelBufferView(
            pendingDescriptorWriteCount);
        for (uint32_t i = 0; i < pendingDescriptorWriteCount; ++i) {
            internal_pPendingDescriptorWrites[i] = pPendingDescriptorWrites[i];
            /* VkWriteDescriptorSet::dstSet */
            VK_FROM_HANDLE(gfxstream_vk_descriptor_set, gfxstream_dstSet,
                           internal_pPendingDescriptorWrites[i].dstSet);
            internal_pPendingDescriptorWrites[i].dstSet = gfxstream_dstSet->internal_object;
            /* VkWriteDescriptorSet::pImageInfo */
            internal_nested_pImageInfo[i].reserve(internal_pPendingDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pPendingDescriptorWrites[i].descriptorCount; ++j) {
                internal_nested_pImageInfo[i][j] = internal_pPendingDescriptorWrites[i].pImageInfo[j];
                VK_FROM_HANDLE(gfxstream_vk_sampler, gfxstream_sampler, internal_nested_pImageInfo[i][j].sampler);
                internal_nested_pImageInfo[i][j].sampler = gfxstream_sampler->internal_object;
                VK_FROM_HANDLE(gfxstream_vk_image_view, gfxstream_imageView, internal_nested_pImageInfo[i][j].imageView);
                internal_nested_pImageInfo[i][j].imageView = gfxstream_imageView->internal_object;
            }
            internal_pPendingDescriptorWrites[i].pImageInfo = internal_nested_pImageInfo[i].data();
            /* VkWriteDescriptorSet::pBufferInfo */
            internal_nested_pBufferInfo[i].reserve(internal_pPendingDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pPendingDescriptorWrites[i].descriptorCount; ++j) {
                internal_nested_pBufferInfo[i][j] = internal_pPendingDescriptorWrites[i].pBufferInfo[j];
                VK_FROM_HANDLE(gfxstream_vk_buffer, gfxstream_buffer, internal_nested_pBufferInfo[i][j].buffer);
                internal_nested_pBufferInfo[i][j].buffer = gfxstream_buffer->internal_object;
            }
            internal_pPendingDescriptorWrites[i].pBufferInfo = internal_nested_pBufferInfo[i].data();
            /* VkWriteDescriptorSet::pTexelBufferView */
            internal_nested_pTexelBufferView[i].reserve(
                internal_pPendingDescriptorWrites[i].descriptorCount);
            for (uint32_t j = 0; j < internal_pPendingDescriptorWrites[i].descriptorCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_buffer_view, gfxstream_pTexelBufferView,
                               internal_pPendingDescriptorWrites[i].pTexelBufferView[j]);
                internal_nested_pTexelBufferView[i][j] =
                    gfxstream_pTexelBufferView->internal_object;
            }
            internal_pPendingDescriptorWrites[i].pTexelBufferView =
                internal_nested_pTexelBufferView[i].data();
        }
        vkEnc->vkQueueCommitDescriptorSetUpdatesGOOGLE(
            gfxstream_queue->internal_object, descriptorPoolCount, internal_pDescriptorPools.data(),
            descriptorSetCount, internal_pSetLayouts.data(), pDescriptorSetPoolIds,
            pDescriptorSetWhichPool, pDescriptorSetPendingAllocation,
            pDescriptorWriteStartingIndices, pendingDescriptorWriteCount,
            internal_pPendingDescriptorWrites.data(), true /* do lock */);
    }
}



#if 0
// TODO: VkBindSparseInfo::VkSparseBufferMemoryBindInfo
// TODO: VkBindSparseInfo::VkSparseImageOpaqueMemoryBindInfo
// TODO: VkBindSparseInfo::VkSparseImageMemoryBindInfo
VkResult gfxstream_vk_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount,
                                      const VkBindSparseInfo* pBindInfo, VkFence fence) {
    AEMU_SCOPED_TRACE("vkQueueBindSparse");
    VK_FROM_HANDLE(gfxstream_vk_queue, gfxstream_queue, queue);
    VK_FROM_HANDLE(gfxstream_vk_fence, gfxstream_fence, fence);
    VkResult vkQueueBindSparse_VkResult_return = (VkResult)0;
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getQueueEncoder(queue);
        std::vector<VkBindSparseInfo> internal_pBindInfo(bindInfoCount);
        std::vector<std::vector<VkSemaphore>> internal_nested_pWaitSemaphores(bindInfoCount);
        std::vector<std::vector<VkSemaphore>> internal_nested_pSignalSemaphores(bindInfoCount);
        for (uint32_t i = 0; i < bindInfoCount; ++i) {
            internal_pBindInfo[i] = pBindInfo[i];
            /* VkBindSparseInfo::pWaitSemaphores */
            internal_nested_pWaitSemaphores[i].reserve(internal_pBindInfo[i].waitSemaphoreCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].waitSemaphoreCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_pWaitSemaphores,
                               internal_pBindInfo[i].pWaitSemaphores[j]);
                internal_nested_pWaitSemaphores[i][j] = gfxstream_pWaitSemaphores->internal_object;
            }
            internal_pBindInfo[i].pWaitSemaphores = internal_nested_pWaitSemaphores[i].data();
            /* VkBindSparseInfo::pBufferBinds */
            internal_nested_pBufferBinds[i].reserve(internal_pBindInfo[i].bufferBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].bufferBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_buffer_memory_bind_info, gfxstream_pBufferBinds,
                               internal_pBindInfo[i].pBufferBinds[j]);
                internal_nested_pBufferBinds[i][j] = gfxstream_pBufferBinds->internal_object;
            }
            internal_pBindInfo[i].pBufferBinds = internal_nested_pBufferBinds[i].data();
            /* VkBindSparseInfo::pImageOpaqueBinds */
            internal_nested_pImageOpaqueBinds[i].reserve(
                internal_pBindInfo[i].imageOpaqueBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].imageOpaqueBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_image_opaque_memory_bind_info,
                               gfxstream_pImageOpaqueBinds,
                               internal_pBindInfo[i].pImageOpaqueBinds[j]);
                internal_nested_pImageOpaqueBinds[i][j] =
                    gfxstream_pImageOpaqueBinds->internal_object;
            }
            internal_pBindInfo[i].pImageOpaqueBinds = internal_nested_pImageOpaqueBinds[i].data();
            /* VkBindSparseInfo::pImageBinds */
            internal_nested_pImageBinds[i].reserve(internal_pBindInfo[i].imageBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].imageBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_image_memory_bind_info, gfxstream_pImageBinds,
                               internal_pBindInfo[i].pImageBinds[j]);
                internal_nested_pImageBinds[i][j] = gfxstream_pImageBinds->internal_object;
            }
            internal_pBindInfo[i].pImageBinds = internal_nested_pImageBinds[i].data();
            /* VkBindSparseInfo::pSignalSemaphores */
            internal_nested_pSignalSemaphores[i].reserve(
                internal_pBindInfo[i].signalSemaphoreCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].signalSemaphoreCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_pSignalSemaphores,
                               internal_pBindInfo[i].pSignalSemaphores[j]);
                internal_nested_pSignalSemaphores[i][j] =
                    gfxstream_pSignalSemaphores->internal_object;
            }
            internal_pBindInfo[i].pSignalSemaphores = internal_nested_pSignalSemaphores[i].data();
        }
        vkQueueBindSparse_VkResult_return = vkEnc->vkQueueBindSparse(
            gfxstream_queue->internal_object, bindInfoCount, internal_pBindInfo.data(),
            gfxstream_fence->internal_object, true /* do lock */);
    }
    return vkQueueBindSparse_VkResult_return;
}

// TODO: VkBindSparseInfo::VkSparseBufferMemoryBindInfo
// TODO: VkBindSparseInfo::VkSparseImageOpaqueMemoryBindInfo
// TODO: VkBindSparseInfo::VkSparseImageMemoryBindInfo
void gfxstream_vk_QueueBindSparseAsyncGOOGLE(VkQueue queue, uint32_t bindInfoCount,
                                             const VkBindSparseInfo* pBindInfo, VkFence fence) {
    AEMU_SCOPED_TRACE("vkQueueBindSparseAsyncGOOGLE");
    VK_FROM_HANDLE(gfxstream_vk_queue, gfxstream_queue, queue);
    VK_FROM_HANDLE(gfxstream_vk_fence, gfxstream_fence, fence);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getQueueEncoder(queue);
        std::vector<VkBindSparseInfo> internal_pBindInfo(bindInfoCount);
        std::vector<std::vector<VkSemaphore>> internal_nested_pWaitSemaphores(bindInfoCount);
        std::vector<std::vector<VkSemaphore>> internal_nested_pSignalSemaphores(bindInfoCount);
        for (uint32_t i = 0; i < bindInfoCount; ++i) {
            internal_pBindInfo[i] = pBindInfo[i];
            /* VkBindSparseInfo::pWaitSemaphores */
            internal_nested_pWaitSemaphores[i].reserve(internal_pBindInfo[i].waitSemaphoreCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].waitSemaphoreCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_pWaitSemaphores,
                               internal_pBindInfo[i].pWaitSemaphores[j]);
                internal_nested_pWaitSemaphores[i][j] = gfxstream_pWaitSemaphores->internal_object;
            }
            internal_pBindInfo[i].pWaitSemaphores = internal_nested_pWaitSemaphores[i].data();
            /* VkBindSparseInfo::pBufferBinds */
            internal_nested_pBufferBinds[i].reserve(internal_pBindInfo[i].bufferBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].bufferBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_buffer_memory_bind_info, gfxstream_pBufferBinds,
                               internal_pBindInfo[i].pBufferBinds[j]);
                internal_nested_pBufferBinds[i][j] = gfxstream_pBufferBinds->internal_object;
            }
            internal_pBindInfo[i].pBufferBinds = internal_nested_pBufferBinds[i].data();
            /* VkBindSparseInfo::pImageOpaqueBinds */
            internal_nested_pImageOpaqueBinds[i].reserve(
                internal_pBindInfo[i].imageOpaqueBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].imageOpaqueBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_image_opaque_memory_bind_info,
                               gfxstream_pImageOpaqueBinds,
                               internal_pBindInfo[i].pImageOpaqueBinds[j]);
                internal_nested_pImageOpaqueBinds[i][j] =
                    gfxstream_pImageOpaqueBinds->internal_object;
            }
            internal_pBindInfo[i].pImageOpaqueBinds = internal_nested_pImageOpaqueBinds[i].data();
            /* VkBindSparseInfo::pImageBinds */
            internal_nested_pImageBinds[i].reserve(internal_pBindInfo[i].imageBindCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].imageBindCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_sparse_image_memory_bind_info, gfxstream_pImageBinds,
                               internal_pBindInfo[i].pImageBinds[j]);
                internal_nested_pImageBinds[i][j] = gfxstream_pImageBinds->internal_object;
            }
            internal_pBindInfo[i].pImageBinds = internal_nested_pImageBinds[i].data();
            /* VkBindSparseInfo::pSignalSemaphores */
            internal_nested_pSignalSemaphores[i].reserve(
                internal_pBindInfo[i].signalSemaphoreCount);
            for (uint32_t j = 0; j < internal_pBindInfo[i].signalSemaphoreCount; ++j) {
                VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_pSignalSemaphores,
                               internal_pBindInfo[i].pSignalSemaphores[j]);
                internal_nested_pSignalSemaphores[i][j] =
                    gfxstream_pSignalSemaphores->internal_object;
            }
            internal_pBindInfo[i].pSignalSemaphores = internal_nested_pSignalSemaphores[i].data();
        }
        vkEnc->vkQueueBindSparseAsyncGOOGLE(gfxstream_queue->internal_object, bindInfoCount,
                                            internal_pBindInfo.data(),
                                            gfxstream_fence->internal_object, true /* do lock */);
    }
}


#endif


#endif
