// Copyright 2022 The Android Open Source Project
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

#include "VkFormatUtils.h"

#include <unordered_map>

namespace gfxstream {
namespace vk {
#include "host-common/logging.h"
#include "vulkan/vk_enum_string_helper.h"

namespace {

struct FormatPlaneLayout {
    uint32_t horizontalSubsampling = 1;
    uint32_t verticalSubsampling = 1;
    uint32_t sampleIncrementBytes = 0;
    VkImageAspectFlags aspectMask = 0;
};

struct FormatPlaneLayouts {
    uint32_t horizontalAlignmentPixels = 1;
    std::vector<FormatPlaneLayout> planeLayouts;
};

const std::unordered_map<VkFormat, FormatPlaneLayouts>& getFormatPlaneLayoutsMap() {
    static const auto* kPlaneLayoutsMap = []() {
        auto* map = new std::unordered_map<VkFormat, FormatPlaneLayouts>({
            {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
             {
                 .horizontalAlignmentPixels = 2,
                 .planeLayouts =
                     {
                         {
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                             .sampleIncrementBytes = 2,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                         },
                         {
                             .horizontalSubsampling = 2,
                             .verticalSubsampling = 2,
                             .sampleIncrementBytes = 4,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                         },
                     },
             }},
            {VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
             {
                 .horizontalAlignmentPixels = 2,
                 .planeLayouts =
                     {
                         {
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                             .sampleIncrementBytes = 1,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                         },
                         {
                             .horizontalSubsampling = 2,
                             .verticalSubsampling = 2,
                             .sampleIncrementBytes = 2,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                         },
                     },
             }},
            {VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
             {
                 .horizontalAlignmentPixels = 1,
                 .planeLayouts =
                     {
                         {
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                             .sampleIncrementBytes = 1,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                         },
                         {
                             .horizontalSubsampling = 2,
                             .verticalSubsampling = 2,
                             .sampleIncrementBytes = 1,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                         },
                         {
                             .horizontalSubsampling = 2,
                             .verticalSubsampling = 2,
                             .sampleIncrementBytes = 1,
                             .aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT,
                         },
                     },
             }},
        });

#define ADD_SINGLE_PLANE_FORMAT_INFO(format, bpp)            \
    (*map)[format] = FormatPlaneLayouts{                     \
        .horizontalAlignmentPixels = 1,                      \
        .planeLayouts =                                      \
            {                                                \
                {                                            \
                    .horizontalSubsampling = 1,              \
                    .verticalSubsampling = 1,                \
                    .sampleIncrementBytes = bpp,             \
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, \
                },                                           \
            },                                               \
    };
        LIST_VK_FORMATS_LINEAR(ADD_SINGLE_PLANE_FORMAT_INFO)
#undef ADD_SINGLE_PLANE_FORMAT_INFO

        return map;
    }();
    return *kPlaneLayoutsMap;
}

inline uint32_t alignToPower2(uint32_t val, uint32_t align) {
    return (val + (align - 1)) & ~(align - 1);
}

}  // namespace

const FormatPlaneLayouts* getFormatPlaneLayouts(VkFormat format) {
    const auto& formatPlaneLayoutsMap = getFormatPlaneLayoutsMap();

    auto it = formatPlaneLayoutsMap.find(format);
    if (it == formatPlaneLayoutsMap.end()) {
        return nullptr;
    }
    return &it->second;
}

bool getFormatTransferInfo(const VkImageCreateInfo& imageInfo,
                           VkDeviceSize* outStagingBufferCopySize,
                           std::vector<VkBufferImageCopy>* outBufferImageCopies) {
    const FormatPlaneLayouts* formatInfo = getFormatPlaneLayouts(imageInfo.format);
    if (formatInfo == nullptr) {
        ERR("Unhandled format: %s [%d]", string_VkFormat(imageInfo.format), imageInfo.format);
        return false;
    }

    if (imageInfo.extent.depth != 1) {
        ERR("Unhandled depth: %d", imageInfo.extent.depth);
        return false;
    }

    const uint32_t alignedWidth =
        alignToPower2(imageInfo.extent.width, formatInfo->horizontalAlignmentPixels);
    const uint32_t alignedHeight = imageInfo.extent.height;
    const uint32_t alignedDepth = 1;
    const uint32_t numMipLevels = imageInfo.mipLevels;
    const uint32_t numArrayLayers = imageInfo.arrayLayers;

    uint32_t cumulativeSize = 0;

    for (uint32_t mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        const uint32_t mipLevelWidth = alignedWidth >> mipLevel;
        const uint32_t mipLevelHeight = alignedHeight >> mipLevel;
        const uint32_t mipLevelDepth = 1;

        for (const FormatPlaneLayout& planeInfo : formatInfo->planeLayouts) {
            const uint32_t currentMipPlaneWidth = mipLevelWidth / planeInfo.horizontalSubsampling;
            const uint32_t currentMipPlaneHeight = mipLevelHeight / planeInfo.verticalSubsampling;
            const uint32_t currentMipPlaneDepth = 1;

            // https://docs.vulkan.org/spec/latest/chapters/copies.html#copies-buffers-images
            const uint32_t blockSize = planeInfo.sampleIncrementBytes;
            const uint32_t rowExtent = currentMipPlaneWidth * blockSize;
            const uint32_t sliceExtent = currentMipPlaneHeight * rowExtent;
            const uint32_t layerExtent = currentMipPlaneDepth * sliceExtent;

            if (outBufferImageCopies) {
                outBufferImageCopies->emplace_back(VkBufferImageCopy{
                    .bufferOffset = cumulativeSize,
                    .bufferRowLength = /*indicates tightly packed*/ 0,
                    .bufferImageHeight = /*indicates tightly packed*/ 0,
                    .imageSubresource =
                        {
                            .aspectMask = planeInfo.aspectMask,
                            .mipLevel = mipLevel,
                            .baseArrayLayer = 0,
                            .layerCount = numArrayLayers,
                        },
                    .imageOffset =
                        {
                            .x = 0,
                            .y = 0,
                            .z = 0,
                        },
                    .imageExtent =
                        {
                            .width = currentMipPlaneWidth,
                            .height = currentMipPlaneHeight,
                            .depth = currentMipPlaneDepth,
                        },
                });
            }

            const uint32_t currentMipPlaneSize = numArrayLayers * layerExtent;
            cumulativeSize += currentMipPlaneSize;
        }
    }

    if (outStagingBufferCopySize) {
        *outStagingBufferCopySize = cumulativeSize;
    }

    return true;
}

bool getFormatTransferInfo(VkFormat format, uint32_t width, uint32_t height,
                           VkDeviceSize* outStagingBufferCopySize,
                           std::vector<VkBufferImageCopy>* outBufferImageCopies) {
    const VkImageCreateInfo imageInfo = {
        .format = format,
        .extent =
            {
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
    };
    return getFormatTransferInfo(imageInfo, outStagingBufferCopySize, outBufferImageCopies);
}

}  // namespace vk
}  // namespace gfxstream
