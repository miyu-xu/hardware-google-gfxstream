// Copyright 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vulkan/VkDecoderSnapshotUtils.h"

#include "VkCommonOperations.h"
#include "VkFormatUtils.h"
#include "host-common/GfxstreamFatalError.h"

namespace gfxstream {
namespace vk {
namespace {

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;

uint32_t GetMemoryType(const PhysicalDeviceInfo& physicalDevice,
                       const VkMemoryRequirements& memoryRequirements,
                       VkMemoryPropertyFlags memoryProperties) {
    const auto& props = physicalDevice.memoryPropertiesHelper->getHostMemoryProperties();
    for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
        if (!(memoryRequirements.memoryTypeBits & (1 << i))) {
            continue;
        }
        if ((props.memoryTypes[i].propertyFlags & memoryProperties) != memoryProperties) {
            continue;
        }
        return i;
    }
    GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
        << "Cannot find memory type for snapshot save " << __func__ << " (" << __FILE__ << ":"
        << __LINE__ << ")";
}

}  // namespace

#define _RUN_AND_CHECK(command)                                                             \
    {                                                                                       \
        if (command)                                                                        \
            GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))                   \
                << "Vulkan snapshot save failed at " << __func__ << " (" << __FILE__ << ":" \
                << __LINE__ << ")";                                                         \
    }

void saveImageContent(android::base::Stream* stream, StateBlock* stateBlock, VkImage image,
                      const ImageInfo* imageInfo) {
    if (imageInfo->layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        return;
    }

    const VkImageCreateInfo& imageCreateInfo = imageInfo->imageCreateInfoShallow;

    // TODO(b/333936705): snapshot multi-sample images
    if (imageCreateInfo.samples != VK_SAMPLE_COUNT_1_BIT) {
        return;
    }

    VkDeviceSize neededStagingBufferSize = 0;
    std::vector<VkBufferImageCopy> neededCopies;
    if (!getFormatTransferInfo(imageCreateInfo, &neededStagingBufferSize, &neededCopies)) {
        ERR("Failed to save image content for snapshot.");
        return;
    }

    VulkanDispatch* dispatch = stateBlock->deviceDispatch;

    const VkBufferCreateInfo stagingBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = neededStagingBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkCreateBuffer(stateBlock->device, &stagingBufferCreateInfo, nullptr,
                                            &stagingBuffer));

    VkMemoryRequirements stagingBufferMemoryRequirements{};
    dispatch->vkGetBufferMemoryRequirements(stateBlock->device, stagingBuffer,
                                            &stagingBufferMemoryRequirements);

    const auto stagingBufferMemoryType =
        GetMemoryType(*stateBlock->physicalDeviceInfo, stagingBufferMemoryRequirements,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Staging memory
    // TODO(b/323064243): reuse staging memory
    const VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkAllocateMemory(stateBlock->device, &stagingBufferMemoryAllocateInfo,
                                              nullptr, &stagingBufferMemory));
    _RUN_AND_CHECK(
        dispatch->vkBindBufferMemory(stateBlock->device, stagingBuffer, stagingBufferMemory, 0));

    const VkCommandBufferAllocateInfo commandBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = stateBlock->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkAllocateCommandBuffers(stateBlock->device, &commandBufferAllocInfo,
                                                      &commandBuffer) != VK_SUCCESS);

    const VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (dispatch->vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
        GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
            << "Failed to start command buffer on snapshot save";
    }

    const VkImageLayout imageLayoutBeforeSave = imageInfo->layout;

    const VkImageAspectFlags imageAspects =
        imageCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            ? VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;

    const VkImageMemoryBarrier imageToTransferSrcBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = imageLayoutBeforeSave,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = imageAspects,
                .baseMipLevel = 0,
                .levelCount = imageCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = imageCreateInfo.arrayLayers,
            },
    };
    dispatch->vkCmdPipelineBarrier(commandBuffer,
                                   /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   /*dependencyFlags=*/0,
                                   /*memoryBarrierCount=*/0,
                                   /*pMemoryBarriers=*/nullptr,
                                   /*bufferMemoryBarrierCount=*/0,
                                   /*pBufferMemoryBarriers=*/nullptr,
                                   /*imageMemoryBarrierCount=*/1,
                                   /*pImageMemoryBarriers-*/ &imageToTransferSrcBarrier);

    dispatch->vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                     stagingBuffer, static_cast<uint32_t>(neededCopies.size()),
                                     neededCopies.data());

    // Cannot really translate it back to VK_IMAGE_LAYOUT_PREINITIALIZED
    if (imageLayoutBeforeSave != VK_IMAGE_LAYOUT_PREINITIALIZED) {
        const VkImageMemoryBarrier imageToOriginalLayoutBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = imageLayoutBeforeSave,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = imageAspects,
                    .baseMipLevel = 0,
                    .levelCount = imageCreateInfo.mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = imageCreateInfo.arrayLayers,
                },
        };
        dispatch->vkCmdPipelineBarrier(commandBuffer,
                                       /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       /*dependencyFlags=*/0,
                                       /*memoryBarrierCount=*/0,
                                       /*pMemoryBarriers=*/nullptr,
                                       /*bufferMemoryBarrierCount=*/0,
                                       /*pBufferMemoryBarriers=*/nullptr,
                                       /*imageMemoryBarrierCount=*/1,
                                       /*pImageMemoryBarriers-*/ &imageToOriginalLayoutBarrier);
    }

    _RUN_AND_CHECK(dispatch->vkEndCommandBuffer(commandBuffer));

    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkCreateFence(stateBlock->device, &fenceCreateInfo, nullptr, &fence));

    // Execute the command to copy image
    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    _RUN_AND_CHECK(dispatch->vkQueueSubmit(stateBlock->queue, 1, &submitInfo, fence));
    _RUN_AND_CHECK(dispatch->vkWaitForFences(stateBlock->device, 1, &fence, VK_TRUE, 3000000000L));
    _RUN_AND_CHECK(dispatch->vkResetFences(stateBlock->device, 1, &fence));

    void* mapped = nullptr;
    _RUN_AND_CHECK(dispatch->vkMapMemory(stateBlock->device, stagingBufferMemory, 0, VK_WHOLE_SIZE,
                                         VkMemoryMapFlags{}, &mapped));

    stream->putBe32(/*successfully saved!*/ 1);
    stream->putBe64(neededStagingBufferSize);
    stream->write(mapped, neededStagingBufferSize);

    dispatch->vkUnmapMemory(stateBlock->device, stagingBufferMemory);

    dispatch->vkDestroyFence(stateBlock->device, fence, nullptr);
    dispatch->vkDestroyBuffer(stateBlock->device, stagingBuffer, nullptr);
    dispatch->vkFreeMemory(stateBlock->device, stagingBufferMemory, nullptr);
    dispatch->vkFreeCommandBuffers(stateBlock->device, stateBlock->commandPool, 1, &commandBuffer);
}

void loadImageContent(android::base::Stream* stream, StateBlock* stateBlock, VkImage image,
                      const ImageInfo* imageInfo) {
    const bool saved = stream->getBe32() == 1;
    if (!saved) {
        WARN("Image was not saved to the snapshot, skipping loading...");
        return;
    }

    const VkDeviceSize savedImageSize = static_cast<VkDeviceSize>(stream->getBe64());

    const VkImageCreateInfo& imageCreateInfo = imageInfo->imageCreateInfoShallow;

    VkDeviceSize neededStagingBufferSize = 0;
    std::vector<VkBufferImageCopy> neededCopies;
    if (!getFormatTransferInfo(imageCreateInfo, &neededStagingBufferSize, &neededCopies)) {
        ERR("Failed to save image content for snapshot.");
        return;
    }

    if (savedImageSize != neededStagingBufferSize) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Snapshot saved image size does not match expected: actual saved size "
            << savedImageSize << " vs expected " << neededStagingBufferSize;
        return;
    }

    if (imageInfo->layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        return;
    }

    VulkanDispatch* dispatch = stateBlock->deviceDispatch;

    const VkBufferCreateInfo stagingBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = neededStagingBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkCreateBuffer(stateBlock->device, &stagingBufferCreateInfo, nullptr,
                                            &stagingBuffer));

    VkMemoryRequirements stagingBufferMemoryRequirements{};
    dispatch->vkGetBufferMemoryRequirements(stateBlock->device, stagingBuffer,
                                            &stagingBufferMemoryRequirements);

    const auto stagingBufferMemoryType =
        GetMemoryType(*stateBlock->physicalDeviceInfo, stagingBufferMemoryRequirements,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Staging memory
    // TODO(b/323064243): reuse staging memory
    const VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkAllocateMemory(stateBlock->device, &stagingBufferMemoryAllocateInfo,
                                              nullptr, &stagingMemory));
    _RUN_AND_CHECK(
        dispatch->vkBindBufferMemory(stateBlock->device, stagingBuffer, stagingMemory, 0));

    void* mapped = nullptr;
    _RUN_AND_CHECK(dispatch->vkMapMemory(stateBlock->device, stagingMemory, 0, VK_WHOLE_SIZE,
                                         VkMemoryMapFlags{}, &mapped));

    stream->read(mapped, neededStagingBufferSize);

    dispatch->vkUnmapMemory(stateBlock->device, stagingMemory);

    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = stateBlock->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkAllocateCommandBuffers(stateBlock->device,
                                                      &commandBufferAllocateInfo,
                                                      &commandBuffer) != VK_SUCCESS);

    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    _RUN_AND_CHECK(dispatch->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS);

    const VkImageAspectFlags imageAspects =
        imageCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            ? VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;

    const VkImageLayout imageLayoutBeforeLoad = imageInfo->layout;

    const VkImageMemoryBarrier imageToTransferDstBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = imageLayoutBeforeLoad,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = imageAspects,
                .baseMipLevel = 0,
                .levelCount = imageCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = imageCreateInfo.arrayLayers,
            },
    };
    dispatch->vkCmdPipelineBarrier(commandBuffer,
                                   /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   /*dependencyFlags=*/0,
                                   /*memoryBarrierCount=*/0,
                                   /*pMemoryBarriers=*/nullptr,
                                   /*bufferMemoryBarrierCount=*/0,
                                   /*pBufferMemoryBarriers=*/nullptr,
                                   /*imageMemoryBarrierCount=*/1,
                                   /*pImageMemoryBarriers-*/ &imageToTransferDstBarrier);

    dispatch->vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     stagingBuffer, static_cast<uint32_t>(neededCopies.size()),
                                     neededCopies.data());

    // Cannot really translate it back to VK_IMAGE_LAYOUT_PREINITIALIZED
    if (imageLayoutBeforeLoad != VK_IMAGE_LAYOUT_PREINITIALIZED) {
        const VkImageMemoryBarrier imageToOriginalLayoutBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = imageLayoutBeforeLoad,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = imageAspects,
                    .baseMipLevel = 0,
                    .levelCount = imageCreateInfo.mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = imageCreateInfo.arrayLayers,
                },
        };
        dispatch->vkCmdPipelineBarrier(commandBuffer,
                                       /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       /*srcStageMask=*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       /*dependencyFlags=*/0,
                                       /*memoryBarrierCount=*/0,
                                       /*pMemoryBarriers=*/nullptr,
                                       /*bufferMemoryBarrierCount=*/0,
                                       /*pBufferMemoryBarriers=*/nullptr,
                                       /*imageMemoryBarrierCount=*/1,
                                       /*pImageMemoryBarriers-*/ &imageToOriginalLayoutBarrier);
    }

    _RUN_AND_CHECK(dispatch->vkEndCommandBuffer(commandBuffer));

    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence = VK_NULL_HANDLE;
    _RUN_AND_CHECK(dispatch->vkCreateFence(stateBlock->device, &fenceCreateInfo, nullptr, &fence));

    // Execute the command to copy image
    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    _RUN_AND_CHECK(dispatch->vkQueueSubmit(stateBlock->queue, 1, &submitInfo, fence));
    _RUN_AND_CHECK(dispatch->vkWaitForFences(stateBlock->device, 1, &fence, VK_TRUE, 3000000000L));
    _RUN_AND_CHECK(dispatch->vkResetFences(stateBlock->device, 1, &fence));

    dispatch->vkDestroyFence(stateBlock->device, fence, nullptr);
    dispatch->vkDestroyBuffer(stateBlock->device, stagingBuffer, nullptr);
    dispatch->vkFreeMemory(stateBlock->device, stagingMemory, nullptr);
    dispatch->vkFreeCommandBuffers(stateBlock->device, stateBlock->commandPool, 1, &commandBuffer);
}

void saveBufferContent(android::base::Stream* stream, StateBlock* stateBlock, VkBuffer buffer,
                       const BufferInfo* bufferInfo) {
    VkBufferUsageFlags requiredUsages =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if ((bufferInfo->usage & requiredUsages) != requiredUsages) {
        return;
    }
    VulkanDispatch* dispatch = stateBlock->deviceDispatch;
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = stateBlock->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    _RUN_AND_CHECK(dispatch->vkAllocateCommandBuffers(stateBlock->device, &allocInfo,
                                                      &commandBuffer) != VK_SUCCESS);
    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence;
    _RUN_AND_CHECK(dispatch->vkCreateFence(stateBlock->device, &fenceCreateInfo, nullptr, &fence));
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<VkDeviceSize>(bufferInfo->size),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer stagingBuffer;
    _RUN_AND_CHECK(
        dispatch->vkCreateBuffer(stateBlock->device, &bufferCreateInfo, nullptr, &stagingBuffer));

    VkMemoryRequirements stagingBufferMemoryRequirements{};
    dispatch->vkGetBufferMemoryRequirements(stateBlock->device, stagingBuffer,
                                            &stagingBufferMemoryRequirements);

    const auto stagingBufferMemoryType =
        GetMemoryType(*stateBlock->physicalDeviceInfo, stagingBufferMemoryRequirements,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Staging memory
    // TODO(b/323064243): reuse staging memory
    VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    VkDeviceMemory stagingBufferMemory;
    _RUN_AND_CHECK(dispatch->vkAllocateMemory(stateBlock->device, &stagingBufferMemoryAllocateInfo,
                                              nullptr, &stagingBufferMemory));
    _RUN_AND_CHECK(
        dispatch->vkBindBufferMemory(stateBlock->device, stagingBuffer, stagingBufferMemory, 0));

    void* mapped = nullptr;
    _RUN_AND_CHECK(dispatch->vkMapMemory(stateBlock->device, stagingBufferMemory, 0, VK_WHOLE_SIZE,
                                         VkMemoryMapFlags{}, &mapped));

    VkBufferCopy bufferCopy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = bufferInfo->size,
    };

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (dispatch->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
            << "Failed to start command buffer on snapshot save";
    }
    dispatch->vkCmdCopyBuffer(commandBuffer, buffer, stagingBuffer, 1, &bufferCopy);
    VkBufferMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                  .pNext = nullptr,
                                  .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                  .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                                  .srcQueueFamilyIndex = 0xFFFFFFFF,
                                  .dstQueueFamilyIndex = 0xFFFFFFFF,
                                  .buffer = stagingBuffer,
                                  .offset = 0,
                                  .size = bufferInfo->size};
    dispatch->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0,
                                   nullptr);

    // Execute the command to copy buffer
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    _RUN_AND_CHECK(dispatch->vkEndCommandBuffer(commandBuffer));
    _RUN_AND_CHECK(dispatch->vkQueueSubmit(stateBlock->queue, 1, &submitInfo, fence));
    _RUN_AND_CHECK(dispatch->vkWaitForFences(stateBlock->device, 1, &fence, VK_TRUE, 3000000000L));
    _RUN_AND_CHECK(dispatch->vkResetFences(stateBlock->device, 1, &fence));
    stream->putBe64(bufferInfo->size);
    stream->write(mapped, bufferInfo->size);

    dispatch->vkDestroyFence(stateBlock->device, fence, nullptr);
    dispatch->vkUnmapMemory(stateBlock->device, stagingBufferMemory);
    dispatch->vkDestroyBuffer(stateBlock->device, stagingBuffer, nullptr);
    dispatch->vkFreeMemory(stateBlock->device, stagingBufferMemory, nullptr);
    dispatch->vkFreeCommandBuffers(stateBlock->device, stateBlock->commandPool, 1, &commandBuffer);
}

void loadBufferContent(android::base::Stream* stream, StateBlock* stateBlock, VkBuffer buffer,
                       const BufferInfo* bufferInfo) {
    VkBufferUsageFlags requiredUsages =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if ((bufferInfo->usage & requiredUsages) != requiredUsages) {
        return;
    }
    VulkanDispatch* dispatch = stateBlock->deviceDispatch;
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = stateBlock->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    _RUN_AND_CHECK(dispatch->vkAllocateCommandBuffers(stateBlock->device, &allocInfo,
                                                      &commandBuffer) != VK_SUCCESS);
    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence;
    _RUN_AND_CHECK(dispatch->vkCreateFence(stateBlock->device, &fenceCreateInfo, nullptr, &fence));
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<VkDeviceSize>(bufferInfo->size),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer stagingBuffer;
    _RUN_AND_CHECK(
        dispatch->vkCreateBuffer(stateBlock->device, &bufferCreateInfo, nullptr, &stagingBuffer));

    VkMemoryRequirements stagingBufferMemoryRequirements{};
    dispatch->vkGetBufferMemoryRequirements(stateBlock->device, stagingBuffer,
                                            &stagingBufferMemoryRequirements);

    const auto stagingBufferMemoryType =
        GetMemoryType(*stateBlock->physicalDeviceInfo, stagingBufferMemoryRequirements,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Staging memory
    // TODO(b/323064243): reuse staging memory
    VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    VkDeviceMemory stagingMemory;
    _RUN_AND_CHECK(dispatch->vkAllocateMemory(stateBlock->device, &stagingBufferMemoryAllocateInfo,
                                              nullptr, &stagingMemory));
    _RUN_AND_CHECK(
        dispatch->vkBindBufferMemory(stateBlock->device, stagingBuffer, stagingMemory, 0));

    void* mapped = nullptr;
    _RUN_AND_CHECK(dispatch->vkMapMemory(stateBlock->device, stagingMemory, 0, VK_WHOLE_SIZE,
                                         VkMemoryMapFlags{}, &mapped));
    size_t bufferSize = stream->getBe64();
    assert(bufferSize == bufferInfo->size);
    stream->read(mapped, bufferInfo->size);

    VkBufferCopy bufferCopy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = bufferInfo->size,
    };

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (dispatch->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
            << "Failed to start command buffer on snapshot save";
    }
    dispatch->vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &bufferCopy);
    VkBufferMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                  .pNext = nullptr,
                                  .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                  .dstAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
                                  .srcQueueFamilyIndex = 0xFFFFFFFF,
                                  .dstQueueFamilyIndex = 0xFFFFFFFF,
                                  .buffer = buffer,
                                  .offset = 0,
                                  .size = bufferInfo->size};
    dispatch->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier,
                                   0, nullptr);

    // Execute the command to copy buffer
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    _RUN_AND_CHECK(dispatch->vkEndCommandBuffer(commandBuffer));
    _RUN_AND_CHECK(dispatch->vkQueueSubmit(stateBlock->queue, 1, &submitInfo, fence));
    _RUN_AND_CHECK(dispatch->vkWaitForFences(stateBlock->device, 1, &fence, VK_TRUE, 3000000000L));
    _RUN_AND_CHECK(dispatch->vkResetFences(stateBlock->device, 1, &fence));

    dispatch->vkDestroyFence(stateBlock->device, fence, nullptr);
    dispatch->vkUnmapMemory(stateBlock->device, stagingMemory);
    dispatch->vkDestroyBuffer(stateBlock->device, stagingBuffer, nullptr);
    dispatch->vkFreeMemory(stateBlock->device, stagingMemory, nullptr);
    dispatch->vkFreeCommandBuffers(stateBlock->device, stateBlock->commandPool, 1, &commandBuffer);
}

}  // namespace vk
}  // namespace gfxstream