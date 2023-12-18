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

#include "vk_sync_dummy.h"

static bool isNoopSemaphore(gfxstream_vk_semaphore* semaphore) {
    /* Under the assumption that Mesa VK runtime queue submission is used, WSI flow
     * sets this temporary state to a dummy sync type (when no explicit dma-buf
     * synchronization is available). For gfxstream case, ignore this semaphore
     * when this is the case. Synchronization will be done on the host.
     */
    return (semaphore && semaphore->vk.temporary &&
            vk_sync_type_is_dummy(semaphore->vk.temporary->type));
}

std::vector<VkSemaphore> transformVkSemaphoreList(const VkSemaphore* pSemaphores,
                                                  uint32_t semaphoreCount) {
    std::vector<VkSemaphore> outSemaphores;
    for (uint32_t j = 0; j < semaphoreCount; ++j) {
        VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_semaphore, pSemaphores[j]);
        if (!isNoopSemaphore(gfxstream_semaphore)) {
            outSemaphores.push_back(gfxstream_semaphore->internal_object);
        }
    }
    return outSemaphores;
}

std::vector<VkSemaphoreSubmitInfo> transformVkSemaphoreSubmitInfoList(
    const VkSemaphoreSubmitInfo* pSemaphoreSubmitInfos, uint32_t semaphoreSubmitInfoCount) {
    std::vector<VkSemaphoreSubmitInfo> outSemaphoreSubmitInfo;
    for (uint32_t j = 0; j < semaphoreSubmitInfoCount; ++j) {
        VkSemaphoreSubmitInfo outInfo = pSemaphoreSubmitInfos[j];
        VK_FROM_HANDLE(gfxstream_vk_semaphore, gfxstream_semaphore, outInfo.semaphore);
        if (!isNoopSemaphore(gfxstream_semaphore)) {
            outInfo.semaphore = gfxstream_semaphore->internal_object;
            outSemaphoreSubmitInfo.push_back(outInfo);
        }
    }
    return outSemaphoreSubmitInfo;
}

static bool vkDescriptorTypeHasImageView(VkDescriptorType type) {
    switch (type) {
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return true;
        default:
            return false;
    }
}

static bool vkDescriptorTypeHasDescriptorBuffer(VkDescriptorType type) {
    switch (type) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return true;
        default:
            return false;
    }
}

static bool vkDescriptorTypeHasTexelBuffer(VkDescriptorType type) {
    switch (type) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return true;
        default:
            return false;
    }
}

std::vector<VkWriteDescriptorSet> transformVkWriteDescriptorSetList(
    const VkWriteDescriptorSet* pDescriptorSets,
    uint32_t descriptorSetCount,
    VkWriteDescriptorSetListTransformStorage& storage) {
    std::vector<VkWriteDescriptorSet> outDescriptorSets(descriptorSetCount);
    for (uint32_t i = 0; i < descriptorSetCount; ++i) {
        /* VkWriteDescriptorSet::pImageInfo */
        storage.imageInfos.push_back(std::vector<VkDescriptorImageInfo>());
        storage.imageInfos[i].reserve(pDescriptorSets[i].descriptorCount);
        memset(&storage.imageInfos[i][0], 0,
               sizeof(VkDescriptorImageInfo) * pDescriptorSets[i].descriptorCount);
        for (uint32_t j = 0; j < pDescriptorSets[i].descriptorCount; ++j) {
            if (pDescriptorSets[i].pImageInfo) {
                storage.imageInfos[i][j] = pDescriptorSets[i].pImageInfo[j];
                /* VkDescriptorImageInfo::imageView */
                if (storage.imageInfos[i][j].imageView) {
                    VK_FROM_HANDLE(gfxstream_vk_image_view, gfxstream_imageView,
                                   storage.imageInfos[i][j].imageView);
                    storage.imageInfos[i][j].imageView = gfxstream_imageView->internal_object;
                }
            }
        }
        outDescriptorSets[i].pImageInfo = storage.imageInfos[i].data();

        /* VkWriteDescriptorSet::pBufferInfo */
        storage.bufferInfos.push_back(std::vector<VkDescriptorBufferInfo>());
        storage.bufferInfos[i].reserve(pDescriptorSets[i].descriptorCount);
        memset(&storage.bufferInfos[i][0], 0,
               sizeof(VkDescriptorBufferInfo) * pDescriptorSets[i].descriptorCount);
        for (uint32_t j = 0; j < pDescriptorSets[i].descriptorCount; ++j) {
            if (pDescriptorSets[i].pBufferInfo) {
                storage.bufferInfos[i][j] = pDescriptorSets[i].pBufferInfo[j];
                /* VkDescriptorBufferInfo::buffer */
                if (storage.bufferInfos[i][j].buffer) {
                    VK_FROM_HANDLE(gfxstream_vk_buffer, gfxstream_buffer,
                                   storage.bufferInfos[i][j].buffer);
                    storage.bufferInfos[i][j].buffer = gfxstream_buffer->internal_object;
                }
            }
        }
        outDescriptorSets[i].pBufferInfo = storage.bufferInfos[i].data();

        /* VkWriteDescriptorSet::pTexelBufferView */
        storage.texelBuffers.push_back(std::vector<VkBufferView>());
        storage.texelBuffers[i].reserve(pDescriptorSets[i].descriptorCount);
        memset(&storage.texelBuffers[i][0], 0,
               sizeof(VkBufferView) * pDescriptorSets[i].descriptorCount);
        for (uint32_t j = 0; j < pDescriptorSets[i].descriptorCount; ++j) {
            if (pDescriptorSets[i].pTexelBufferView) {
                VK_FROM_HANDLE(gfxstream_vk_buffer_view, gfxstream_pTexelBufferView,
                               pDescriptorSets[i].pTexelBufferView[j]);
                storage.texelBuffers[i][j] = gfxstream_pTexelBufferView->internal_object;
            }
        }
        outDescriptorSets[i].pTexelBufferView = storage.texelBuffers[i].data();
    }

    return outDescriptorSets;
}
