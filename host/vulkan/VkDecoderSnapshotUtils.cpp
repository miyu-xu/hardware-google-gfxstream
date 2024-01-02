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

namespace gfxstream {
namespace vk {

namespace {

uint32_t GetMemoryType(const PhysicalDeviceInfo& physicalDevice,
                       const VkMemoryRequirements& memoryRequirements,
                       VkMemoryPropertyFlags memoryProperties) {
    const auto& props = physicalDevice.memoryProperties;
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

uint32_t bytes_per_pixel(VkFormat format) {
	switch (format) {
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
		return 1;
	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
		return 2;
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
		return 3;
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SNORM:
	case VK_FORMAT_R8G8B8A8_USCALED:
	case VK_FORMAT_R8G8B8A8_SSCALED:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_USCALED:
	case VK_FORMAT_B8G8R8A8_SSCALED:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		return 4;
	default:
		return 0;
	}
}
} // namespace

void saveImageContent(android::base::Stream* stream, StateBlock* stateBlock, VkImage image, const ImageInfo* imageInfo) {
	const VkImageCreateInfo& imageCreateInfo = imageInfo->imageCreateInfoShallow;
	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = stateBlock->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commandBuffer;
	if (stateBlock->dispatch->vkAllocateCommandBuffers(stateBlock->device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
	    GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
	    	<< "Failed to create command buffer on snapshot save";
	}
	VkFenceCreateInfo fenceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};
	VkFence fence;
	stateBlock->dispatch->vkCreateFence(stateBlock->device, &fenceCreateInfo, nullptr, &fence);
    VkBufferCreateInfo bufferCreateInfo = {
        .size = static_cast<VkDeviceSize>(imageCreateInfo.extent.width * imageCreateInfo.extent.height
        	* imageCreateInfo.extent.depth * bytes_per_pixel(imageCreateInfo.format)),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer readbackBuffer;
    stateBlock->dispatch->vkCreateBuffer(stateBlock->device, &bufferCreateInfo, nullptr, &readbackBuffer);

    VkMemoryRequirements readbackBufferMemoryRequirements{};
    stateBlock->dispatch->vkGetBufferMemoryRequirements(
    	stateBlock->device, readbackBuffer, &readbackBufferMemoryRequirements);

    const auto readbackBufferMemoryType =
         GetMemoryType(stateBlock->physicalDeviceInfo,
                       readbackBufferMemoryRequirements,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Staging memory
    // TODO: cache it? create a pool for it?
    VkMemoryAllocateInfo readbackBufferMemoryAllocateInfo = {
        .allocationSize = readbackBufferMemoryRequirements.size,
        .memoryTypeIndex = readbackBufferMemoryType,
    };
    VkDeviceMemory readbackMemory;
    stateBlock->dispatch->vkAllocateMemory(stateBlock->device, &readbackBufferMemoryAllocateInfo, nullptr, &readbackMemory);
    stateBlock->dispatch->vkBindBufferMemory(stateBlock->device, readbackBuffer, readbackMemory, 0);

    void* mapped = nullptr;
    stateBlock->dispatch->vkMapMemory(stateBlock->device, readbackMemory, 0, VK_WHOLE_SIZE, VkMemoryMapFlags{}, &mapped);

	for (uint32_t mipLevel = 0; mipLevel < imageInfo->imageCreateInfoShallow.mipLevels; mipLevel ++) {
		for (uint32_t arrayLayer = 0; arrayLayer < imageInfo->imageCreateInfoShallow.arrayLayers; arrayLayer ++) {
			// TODO: use fewer command buffers
			VkCommandBufferBeginInfo beginInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};
			if (stateBlock->dispatch->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			    GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
			    	<< "Failed to start command buffer on snapshot save";
			}

			// TODO: get the right aspect
			VkImageAspectFlags aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageLayout layoutBeforeSave = imageInfo->layout;
			VkImageMemoryBarrier imgMemoryBarrier = {
		      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		      .pNext = nullptr,
		      .srcAccessMask = static_cast<VkAccessFlags>(~VK_ACCESS_NONE_KHR),
		      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		      .oldLayout = layoutBeforeSave,
		      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		      .image = image,
		      .subresourceRange = VkImageSubresourceRange{
		          .aspectMask = aspects,
		          .baseMipLevel = mipLevel,
		          .levelCount = 1,
		          .baseArrayLayer = arrayLayer,
		          .layerCount = 1}};

			stateBlock->dispatch->vkCmdPipelineBarrier(
			    commandBuffer,
			    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			    0, 0, nullptr, 0, nullptr, 1, &imgMemoryBarrier);

			VkBufferImageCopy region {
		        .bufferOffset = 0,
		        .bufferRowLength = 0,
		        .bufferImageHeight = 0,
		        .imageSubresource = VkImageSubresourceLayers{
		            .aspectMask = aspects,
		            .mipLevel = mipLevel,
		            .baseArrayLayer = arrayLayer,
		            .layerCount = 1},
		        .imageOffset = VkOffset3D {
		        	.x = 0,
		        	.y = 0,
		        	.z = 0,
		        },
		        .imageExtent = imageInfo->imageCreateInfoShallow.extent,
		    };
    		stateBlock->dispatch->vkCmdCopyImageToBuffer(commandBuffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer, 1,
                               &region);

			imgMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imgMemoryBarrier.newLayout = layoutBeforeSave;
			imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imgMemoryBarrier.dstAccessMask = ~VK_ACCESS_NONE_KHR;

			stateBlock->dispatch->vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0, 0, nullptr, 0, nullptr, 1, &imgMemoryBarrier);
			stateBlock->dispatch->vkEndCommandBuffer(commandBuffer);

			// Execute the command to copy image
			VkSubmitInfo submitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			    .commandBufferCount = 1,
			    .pCommandBuffers = &commandBuffer,
			};
			stateBlock->dispatch->vkQueueSubmit(queue, 1, &submitInfo, fence);
			if (stateBlock->dispatch->vkWaitForFences(stateBlock->device, 1, &fence, 3000000000L) != VK_SUCCESS) {
			    GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
			    	<< "Copy image to stage buffer times out";
			}
			stateBlock->dispatch->vkResetFences(stateBlock->device, 1, &fence);
		}
	}
	stateBlock->dispatch->vkDestroyFence(stateBlock->device, fence, nullptr);
}
}  // namespace vk
}  // namespace gfxstream