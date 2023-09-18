/*
 * Copyright © 2023 Google Inc.
 *
 * derived from panvk_private.h driver which is:
 * Copyright © 2021 Collabora Ltd.
 * Copyright © 2016 Red Hat.
 * Copyright © 2016 Bas Nieuwenhuizen
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef GFXSTREAM_VK_PRIVATE_H
#define GFXSTREAM_VK_PRIVATE_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vk_alloc.h"
#include "vk_buffer.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_descriptor_set_layout.h"
#include "vk_device.h"
#include "vk_image.h"
#include "vk_instance.h"
#include "vk_log.h"
#include "vk_object.h"
#include "vk_physical_device.h"
#include "vk_pipeline_layout.h"
#include "vk_queue.h"
#include "vk_sync.h"

#include "vulkan/wsi/wsi_common.h"

#include "vk_extensions.h"

#include <vulkan/vk_icd.h>
#include <vulkan/vulkan.h>

#include "gfxstream_vk_entrypoints.h"

struct gfxstream_vk_physical_device {
   struct vk_physical_device vk;
   struct wsi_device wsi_device;
   VkPhysicalDevice internal_object;
};

struct gfxstream_vk_instance {
   struct vk_instance vk;
   uint32_t api_version;
   VkInstance internal_object;
};

struct gfxstream_vk_pipeline_cache {
   struct vk_object_base base;
   VkAllocationCallbacks alloc;
   VkInstance internal_object;
};

struct gfxstream_vk_queue {
   struct vk_queue vk;
   struct gfxstream_vk_device *device;
   VkQueue internal_object;
};

struct gfxstream_vk_device {
   struct vk_device vk;

   struct vk_device_dispatch_table cmd_dispatch;
   struct gfxstream_vk_instance *instance;
   struct gfxstream_vk_physical_device *physical_device;
   VkDevice internal_object;
};

struct gfxstream_vk_device_memory {
   struct vk_object_base base;
   VkDeviceMemory internal_object;
};

struct gfxstream_vk_descriptor_set {
   struct vk_object_base base;
   VkDescriptorSet internal_object;
};

struct gfxstream_vk_descriptor_set_layout {
   struct vk_descriptor_set_layout vk;
   VkDescriptorSetLayout internal_object;
};

struct gfxstream_vk_pipeline_layout {
   struct vk_pipeline_layout vk;
   VkPipelineLayout internal_object;
};

struct gfxstream_vk_descriptor_pool {
   struct vk_object_base base;
   VkDescriptorPool internal_object;
};

struct gfxstream_vk_buffer {
   struct vk_buffer vk;
   VkBuffer internal_object;
};

struct gfxstream_vk_cmd_pool {
   struct vk_command_pool vk;
   VkCommandPool internal_object;
};

struct gfxstream_vk_cmd_buffer {
   struct vk_command_buffer vk;
   VkCommandBuffer internal_object;
};

struct gfxstream_vk_event {
   struct vk_object_base base;
   VkEvent internal_object;
};

struct gfxstream_vk_pipeline {
   struct vk_object_base base;
   VkPipeline internal_object;
};

struct gfxstream_vk_image {
   struct vk_image vk;
   VkImage internal_object;
};

struct gfxstream_vk_image_view {
   struct vk_image_view vk;
   VkImageView internal_object;
};

struct gfxstream_vk_sampler {
   struct vk_object_base base;
   VkSampler internal_object;
};

struct gfxstream_vk_buffer_view {
   struct vk_object_base base;
   VkBufferView internal_object;
};

struct gfxstream_vk_framebuffer {
   struct vk_object_base base;
   VkFramebuffer internal_object;
};

struct gfxstream_vk_render_pass {
   struct vk_object_base base;
   VkRenderPass internal_object;
};

VK_DEFINE_HANDLE_CASTS(gfxstream_vk_cmd_buffer, vk.base, VkCommandBuffer,
                       VK_OBJECT_TYPE_COMMAND_BUFFER)
VK_DEFINE_HANDLE_CASTS(gfxstream_vk_device, vk.base, VkDevice, VK_OBJECT_TYPE_DEVICE)
VK_DEFINE_HANDLE_CASTS(gfxstream_vk_instance, vk.base, VkInstance,
                       VK_OBJECT_TYPE_INSTANCE)
VK_DEFINE_HANDLE_CASTS(gfxstream_vk_physical_device, vk.base, VkPhysicalDevice,
                       VK_OBJECT_TYPE_PHYSICAL_DEVICE)
VK_DEFINE_HANDLE_CASTS(gfxstream_vk_queue, vk.base, VkQueue, VK_OBJECT_TYPE_QUEUE)

VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_cmd_pool, vk.base, VkCommandPool,
                               VK_OBJECT_TYPE_COMMAND_POOL)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_buffer, vk.base, VkBuffer,
                               VK_OBJECT_TYPE_BUFFER)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_buffer_view, base, VkBufferView,
                               VK_OBJECT_TYPE_BUFFER_VIEW)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_descriptor_pool, base, VkDescriptorPool,
                               VK_OBJECT_TYPE_DESCRIPTOR_POOL)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_descriptor_set, base, VkDescriptorSet,
                               VK_OBJECT_TYPE_DESCRIPTOR_SET)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_descriptor_set_layout, vk.base,
                               VkDescriptorSetLayout,
                               VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_device_memory, base, VkDeviceMemory,
                               VK_OBJECT_TYPE_DEVICE_MEMORY)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_event, base, VkEvent, VK_OBJECT_TYPE_EVENT)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_framebuffer, base, VkFramebuffer,
                               VK_OBJECT_TYPE_FRAMEBUFFER)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_image, vk.base, VkImage,
                               VK_OBJECT_TYPE_IMAGE)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_image_view, vk.base, VkImageView,
                               VK_OBJECT_TYPE_IMAGE_VIEW);
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_pipeline_cache, base, VkPipelineCache,
                               VK_OBJECT_TYPE_PIPELINE_CACHE)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_pipeline, base, VkPipeline,
                               VK_OBJECT_TYPE_PIPELINE)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_pipeline_layout, vk.base, VkPipelineLayout,
                               VK_OBJECT_TYPE_PIPELINE_LAYOUT)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_render_pass, base, VkRenderPass,
                               VK_OBJECT_TYPE_RENDER_PASS)
VK_DEFINE_NONDISP_HANDLE_CASTS(gfxstream_vk_sampler, base, VkSampler,
                               VK_OBJECT_TYPE_SAMPLER)

//VkResult gfxstream_vk_wsi_init(struct gfxstream_vk_physical_device *physical_device);

//void gfxstream_vk_wsi_finish(struct gfxstream_vk_physical_device *physical_device);

//bool gfxstream_vk_instance_extension_supported(const char *name);

//uint32_t gfxstream_vk_physical_device_api_version(struct gfxstream_vk_physical_device *dev);

//bool gfxstream_vk_physical_device_extension_supported(struct gfxstream_vk_physical_device *dev,
//                                                      const char *name);

#endif /* GFXSTREAM_VK_PRIVATE_H */
