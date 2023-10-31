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

#include "gfxstream_vk_private.h"
#include "gfxstream_vk_entrypoints.h"

#include "vk_alloc.h"
#include "vk_device.h"
#include "vk_instance.h"
#include "vk_sync_dummy.h"

#include "ResourceTracker.h"
#include "VkEncoder.h"

#include "../vulkan_enc/vk_util.h"

#include "HostConnection.h"

VkResult SetupInstance(void);

#define VK_HOST_CONNECTION(ret)                                                    \
    HostConnection* hostCon = HostConnection::getOrCreate(kCapsetGfxStreamVulkan); \
    gfxstream::vk::VkEncoder* vkEnc = hostCon->vkEncoder();                        \
    if (!vkEnc) {                                                                  \
        ALOGE("vulkan: Failed to get Vulkan encoder\n");                           \
        return ret;                                                                \
    }

static void get_device_extensions(VkPhysicalDevice physDevInternal, struct vk_device_extension_table *deviceExts) {
    VkResult result = (VkResult)0;
    auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
    auto resources = gfxstream::vk::ResourceTracker::get();
    uint32_t numDeviceExts = 0;
    result = resources->on_vkEnumerateDeviceExtensionProperties(
                vkEnc, VK_SUCCESS, physDevInternal, NULL, &numDeviceExts, NULL);
    if (VK_SUCCESS == result) {
        std::vector<VkExtensionProperties> extProps(numDeviceExts);
        result = resources->on_vkEnumerateDeviceExtensionProperties(
                    vkEnc, VK_SUCCESS, physDevInternal, NULL, &numDeviceExts, extProps.data());
        if (VK_SUCCESS == result) {
            for (uint32_t i = 0; i < numDeviceExts; i++) {
                for (uint32_t j = 0; j < VK_DEVICE_EXTENSION_COUNT; j++) {
                    if (0 == strncmp(extProps[i].extensionName, vk_device_extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE)) {
                        deviceExts->extensions[j] = true;
                        break;
                    }
                }
            }
        }
    }
}

static VkResult gfxstream_vk_physical_device_init(struct gfxstream_vk_physical_device *physical_device, struct gfxstream_vk_instance *instance, VkPhysicalDevice internal_object) {
    struct vk_device_extension_table supported_extensions = {0};
    get_device_extensions(internal_object, &supported_extensions);

    struct vk_physical_device_dispatch_table dispatch_table;
    memset(&dispatch_table, 0, sizeof(struct vk_physical_device_dispatch_table));
    vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table, &gfxstream_vk_physical_device_entrypoints, false);
    vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table, &wsi_physical_device_entrypoints, false);

    // Initialize the mesa object
    VkResult result = vk_physical_device_init(&physical_device->vk, &instance->vk, &supported_extensions, NULL, NULL, &dispatch_table);

    if (VK_SUCCESS == result) {
        // Set the gfxstream-internal object
        physical_device->internal_object = internal_object;
        physical_device->instance = instance;
        // Note: Must use dummy_sync for correct sync object path in WSI operations
        physical_device->sync_types[0] = &vk_sync_dummy_type;
        physical_device->sync_types[1] = NULL;
        physical_device->vk.supported_sync_types = physical_device->sync_types;

        result = gfxstream_vk_wsi_init(physical_device);
    }

    return result;
}

static void
gfxstream_vk_physical_device_finish(struct gfxstream_vk_physical_device *physical_device)
{
   gfxstream_vk_wsi_finish(physical_device);

   vk_physical_device_finish(&physical_device->vk);
}

static void
gfxstream_vk_destroy_physical_device(struct vk_physical_device *physical_device)
{
   gfxstream_vk_physical_device_finish((struct gfxstream_vk_physical_device *)physical_device);
   vk_free(&physical_device->instance->alloc, physical_device);
}

static VkResult
gfxstream_vk_enumerate_devices(struct vk_instance *vk_instance) {
    VkResult result = VK_SUCCESS;
    gfxstream_vk_instance *gfxstream_instance = (gfxstream_vk_instance*)vk_instance;
    uint32_t deviceCount = 0;
    auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
    auto resources = gfxstream::vk::ResourceTracker::get();
    result = resources->on_vkEnumeratePhysicalDevices(
        vkEnc, VK_SUCCESS, gfxstream_instance->internal_object, &deviceCount, NULL);
    if (VK_SUCCESS != result)
        return result;
    std::vector<VkPhysicalDevice> internal_list(deviceCount);
    result = resources->on_vkEnumeratePhysicalDevices(
        vkEnc, VK_SUCCESS, gfxstream_instance->internal_object, &deviceCount, internal_list.data());

    if (VK_SUCCESS == result) {
        for (uint32_t i = 0; i < deviceCount; i++) {
            struct gfxstream_vk_physical_device* gfxstream_physicalDevice = (struct gfxstream_vk_physical_device*)vk_zalloc(
                &gfxstream_instance->vk.alloc,
                sizeof(struct gfxstream_vk_physical_device),
                8,
                VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
            if (!gfxstream_physicalDevice) {
                result = VK_ERROR_OUT_OF_HOST_MEMORY;
                break;
            }
            result = gfxstream_vk_physical_device_init(gfxstream_physicalDevice, gfxstream_instance, internal_list[i]);
            if (VK_SUCCESS == result) {
                list_addtail(&gfxstream_physicalDevice->vk.link, &gfxstream_instance->vk.physical_devices.list);
            } else {
                vk_free(&gfxstream_instance->vk.alloc, gfxstream_physicalDevice);
                break;
            }
        }
    }

    return result;
}

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
                    if (0 == strncmp(extProps[i].extensionName, vk_instance_extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE)) {
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
   vk_instance_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_instance_entrypoints, false);

    struct vk_instance_extension_table supported_extensions;
    get_instance_extensions(&supported_extensions);

    result = vk_instance_init(&instance->vk, &supported_extensions,
                                &dispatch_table, pCreateInfo, pAllocator);

    if (result != VK_SUCCESS) {
        vk_free(pAllocator, instance);
        return vk_error(NULL, result);
    }

    instance->vk.physical_devices.enumerate = gfxstream_vk_enumerate_devices;
    instance->vk.physical_devices.destroy = gfxstream_vk_destroy_physical_device;
    // TODO: instance->vk.physical_devices.try_create_for_drm (?)

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
        vk_device_dispatch_table_from_entrypoints(&dispatch_table, &wsi_device_entrypoints, false);

        result = vk_device_init(&gfxstream_device->vk, &gfxstream_physicalDevice->vk, &dispatch_table, pCreateInfo, pMesaAllocator);
    }
    if (VK_SUCCESS == result) {
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

    /* Must destroy device queues manually */
    vk_foreach_queue_safe(queue, &gfxstream_device->vk) {
        vk_queue_finish(queue);
        vk_free(&gfxstream_device->vk.alloc, queue);
    }
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

PFN_vkVoidFunction
gfxstream_vk_GetInstanceProcAddr(VkInstance _instance, const char *pName)
{
    VK_FROM_HANDLE(gfxstream_vk_instance, instance, _instance);
    return vk_instance_get_proc_addr(&instance->vk, &gfxstream_vk_instance_entrypoints,
                                    pName);
}

PFN_vkVoidFunction
gfxstream_vk_GetDeviceProcAddr(VkDevice _device, const char *pName)
{
    AEMU_SCOPED_TRACE("vkGetDeviceProcAddr");
    VK_FROM_HANDLE(gfxstream_vk_device, device, _device);
    return vk_device_get_proc_addr(&device->vk, pName);
}

VkResult gfxstream_vk_AllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkDeviceMemory* pMemory) {
    AEMU_SCOPED_TRACE("vkAllocateMemory");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VkResult vkAllocateMemory_VkResult_return = (VkResult)0;
    struct gfxstream_vk_device_memory* gfxstream_pMemory =
        (struct gfxstream_vk_device_memory*)vk_device_memory_create(
            (vk_device*)gfxstream_device, pAllocateInfo, pAllocator,
            sizeof(struct gfxstream_vk_device_memory));
    /* VkMemoryDedicatedAllocateInfo */
    VkMemoryDedicatedAllocateInfo* dedicatedAllocInfoPtr =
        (VkMemoryDedicatedAllocateInfo*)gfxstream::vk::vk_find_struct<VkMemoryDedicatedAllocateInfo>(pAllocateInfo);
    if (dedicatedAllocInfoPtr) {
        if (dedicatedAllocInfoPtr->buffer) {
            VK_FROM_HANDLE(gfxstream_vk_buffer, gfxstream_buffer, dedicatedAllocInfoPtr->buffer);
            dedicatedAllocInfoPtr->buffer = gfxstream_buffer->internal_object;
        }
        if (dedicatedAllocInfoPtr->image) {
            VK_FROM_HANDLE(gfxstream_vk_image, gfxstream_image, dedicatedAllocInfoPtr->image);
            dedicatedAllocInfoPtr->image = gfxstream_image->internal_object;
        }
    }
    vkAllocateMemory_VkResult_return = gfxstream_pMemory ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
    if (VK_SUCCESS == vkAllocateMemory_VkResult_return) {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        auto resources = gfxstream::vk::ResourceTracker::get();
        vkAllocateMemory_VkResult_return = resources->on_vkAllocateMemory(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, pAllocateInfo, pAllocator,
            &gfxstream_pMemory->internal_object);
    }
    *pMemory = gfxstream_vk_device_memory_to_handle(gfxstream_pMemory);
    return vkAllocateMemory_VkResult_return;
}

void gfxstream_vk_CmdBeginRenderPass(VkCommandBuffer commandBuffer,
                                     const VkRenderPassBeginInfo* pRenderPassBegin,
                                     VkSubpassContents contents) {
    AEMU_SCOPED_TRACE("vkCmdBeginRenderPass");
    VK_FROM_HANDLE(gfxstream_vk_command_buffer, gfxstream_commandBuffer, commandBuffer);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getCommandBufferEncoder(
            gfxstream_commandBuffer->internal_object);
        VkRenderPassBeginInfo internal_pRenderPassBegin = gfxstream::vk::vk_make_orphan_copy(*pRenderPassBegin);
        gfxstream::vk::vk_struct_chain_iterator structChainIter = gfxstream::vk::vk_make_chain_iterator(&internal_pRenderPassBegin);
        /* VkRenderPassBeginInfo::renderPass */
        VK_FROM_HANDLE(gfxstream_vk_render_pass, gfxstream_renderPass,
                        internal_pRenderPassBegin.renderPass);
        internal_pRenderPassBegin.renderPass = gfxstream_renderPass->internal_object;
        /* VkRenderPassBeginInfo::framebuffer */
        VK_FROM_HANDLE(gfxstream_vk_framebuffer, gfxstream_framebuffer,
                        internal_pRenderPassBegin.framebuffer);
        internal_pRenderPassBegin.framebuffer = gfxstream_framebuffer->internal_object;
        /* pNext = VkRenderPassAttachmentBeginInfo */
        std::vector<VkImageView> internal_pAttachments;
        VkRenderPassAttachmentBeginInfo internal_renderPassAttachmentBeginInfo;
        VkRenderPassAttachmentBeginInfo *pRenderPassAttachmentBeginInfo =
            (VkRenderPassAttachmentBeginInfo*)gfxstream::vk::vk_find_struct<VkRenderPassAttachmentBeginInfo>(pRenderPassBegin);
        if (pRenderPassAttachmentBeginInfo) {
            internal_renderPassAttachmentBeginInfo = *pRenderPassAttachmentBeginInfo;
            /* VkRenderPassAttachmentBeginInfo::pAttachments */
            internal_pAttachments.reserve(internal_renderPassAttachmentBeginInfo.attachmentCount);
            for (uint32_t i = 0; i < internal_renderPassAttachmentBeginInfo.attachmentCount; i++) {
                VK_FROM_HANDLE(gfxstream_vk_image_view, gfxstream_image_view, internal_renderPassAttachmentBeginInfo.pAttachments[i]);
                internal_pAttachments[i] = gfxstream_image_view->internal_object;
            }
            internal_renderPassAttachmentBeginInfo.pAttachments = internal_pAttachments.data();
            vk_append_struct(&structChainIter, &internal_renderPassAttachmentBeginInfo);
        }
        vkEnc->vkCmdBeginRenderPass(gfxstream_commandBuffer->internal_object,
                                    &internal_pRenderPassBegin, contents, true /* do lock */);
    }
}

void gfxstream_vk_CmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer,
                                         const VkRenderPassBeginInfo* pRenderPassBegin,
                                         const VkSubpassBeginInfo* pSubpassBeginInfo) {
    AEMU_SCOPED_TRACE("vkCmdBeginRenderPass2KHR");
    VK_FROM_HANDLE(gfxstream_vk_command_buffer, gfxstream_commandBuffer, commandBuffer);
    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getCommandBufferEncoder(
            gfxstream_commandBuffer->internal_object);
        VkRenderPassBeginInfo internal_pRenderPassBegin = gfxstream::vk::vk_make_orphan_copy(*pRenderPassBegin);
        gfxstream::vk::vk_struct_chain_iterator structChainIter = gfxstream::vk::vk_make_chain_iterator(&internal_pRenderPassBegin);
        /* VkRenderPassBeginInfo::renderPass */
        VK_FROM_HANDLE(gfxstream_vk_render_pass, gfxstream_renderPass,
                        internal_pRenderPassBegin.renderPass);
        internal_pRenderPassBegin.renderPass = gfxstream_renderPass->internal_object;
        /* VkRenderPassBeginInfo::framebuffer */
        VK_FROM_HANDLE(gfxstream_vk_framebuffer, gfxstream_framebuffer,
                        internal_pRenderPassBegin.framebuffer);
        internal_pRenderPassBegin.framebuffer = gfxstream_framebuffer->internal_object;
        /* pNext = VkRenderPassAttachmentBeginInfo */
        std::vector<VkImageView> internal_pAttachments;
        VkRenderPassAttachmentBeginInfo internal_renderPassAttachmentBeginInfo;
        VkRenderPassAttachmentBeginInfo *pRenderPassAttachmentBeginInfo =
            (VkRenderPassAttachmentBeginInfo*)gfxstream::vk::vk_find_struct<VkRenderPassAttachmentBeginInfo>(pRenderPassBegin);
        if (pRenderPassAttachmentBeginInfo) {
            internal_renderPassAttachmentBeginInfo = *pRenderPassAttachmentBeginInfo;
            /* VkRenderPassAttachmentBeginInfo::pAttachments */
            internal_pAttachments.reserve(internal_renderPassAttachmentBeginInfo.attachmentCount);
            for (uint32_t i = 0; i < internal_renderPassAttachmentBeginInfo.attachmentCount; i++) {
                VK_FROM_HANDLE(gfxstream_vk_image_view, gfxstream_image_view, internal_renderPassAttachmentBeginInfo.pAttachments[i]);
                internal_pAttachments[i] = gfxstream_image_view->internal_object;
            }
            internal_renderPassAttachmentBeginInfo.pAttachments = internal_pAttachments.data();
            vk_append_struct(&structChainIter, &internal_renderPassAttachmentBeginInfo);
        }
        vkEnc->vkCmdBeginRenderPass2KHR(gfxstream_commandBuffer->internal_object,
                                        &internal_pRenderPassBegin, pSubpassBeginInfo,
                                        true /* do lock */);
    }
}

VkResult gfxstream_vk_GetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo,
                                     int* pFd) {
    AEMU_SCOPED_TRACE("vkGetMemoryFdKHR");
    VK_FROM_HANDLE(gfxstream_vk_device, gfxstream_device, device);
    VkResult vkGetMemoryFdKHR_VkResult_return = (VkResult)0;

    {
        auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder();
        std::vector<VkMemoryGetFdInfoKHR> internal_pGetFdInfo(1);
        for (uint32_t i = 0; i < 1; ++i) {
            internal_pGetFdInfo[i] = pGetFdInfo[i];
            /* VkMemoryGetFdInfoKHR::memory */
            VK_FROM_HANDLE(gfxstream_vk_device_memory, gfxstream_memory,
                           internal_pGetFdInfo[i].memory);
            internal_pGetFdInfo[i].memory = gfxstream_memory->internal_object;
        }
        auto resources = gfxstream::vk::ResourceTracker::get();
        vkGetMemoryFdKHR_VkResult_return = resources->on_vkGetMemoryFdKHR(
            vkEnc, VK_SUCCESS, gfxstream_device->internal_object, internal_pGetFdInfo.data(), pFd);
    }
    return vkGetMemoryFdKHR_VkResult_return;
}
