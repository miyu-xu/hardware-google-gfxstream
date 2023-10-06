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

// TODO: Only DescriptorSets left in this file (as gfxstream_vk_ entrypoints)
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
