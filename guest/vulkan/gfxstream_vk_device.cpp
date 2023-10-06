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

#include "ResourceTracker.h"
#include "VkEncoder.h"

#include "../vulkan_enc/vk_util.h"

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
                    if ((extProps[i].specVersion == vk_device_extensions[j].specVersion)
                            && (0 == strncmp(extProps[i].extensionName, vk_device_extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE))) {
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
    }

    return result;
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

        if (VK_SUCCESS == result) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                result = gfxstream_vk_physical_device_init(&gfxstream_physicalDevices[i], gfxstream_instance, internal_objects_pointer[i]);
                if (VK_SUCCESS != result) {
                    break;
                }
                // TODO: Add list of physicalDevice enumeration allocations to the instance object
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
                for (uint32_t device = 0; device < pPhysicalDeviceGroupProperties[group].physicalDeviceCount; device++) {
                    result = gfxstream_vk_physical_device_init(&gfxstream_physicalDevices[device], gfxstream_instance, pPhysicalDeviceGroupProperties[group].physicalDevices[device]);
                    if (VK_SUCCESS != result) {
                        break;
                    }
                    // TODO: Add list of physicalDevice enumeration allocations to the instance object
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
