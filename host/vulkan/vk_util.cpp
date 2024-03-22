/*
 * Copyright (C) 2011-2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vk_util.h"
#include "VulkanDispatch.h"
#include "host-common/opengl/misc.h"

namespace gfxstream {
namespace vk {
namespace vk_util {
namespace {

std::unique_ptr<CallbacksWrapper<VkCheckCallbacks>> gVkCheckCallbacks =
    std::make_unique<CallbacksWrapper<VkCheckCallbacks>>(nullptr);

}  // namespace

void setVkCheckCallbacks(std::unique_ptr<VkCheckCallbacks> callbacks) {
    gVkCheckCallbacks = std::make_unique<CallbacksWrapper<VkCheckCallbacks>>(std::move(callbacks));
}

const CallbacksWrapper<VkCheckCallbacks>& getVkCheckCallbacks() { return *gVkCheckCallbacks; }

bool detectVulkanDriverApiVersion() {
    VulkanDispatch* vk = vkDispatch(true);
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Graphics Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "fun engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 1;
    const char** extensions = (const char**)(malloc(sizeof(const char*) * 1));
    extensions[0] = "VK_KHR_surface";
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkInstance instance;

    VkResult result = vk->vkCreateInstance(&createInfo, 0, &instance);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "failed to create instance %d\n", result);
        return false;
    } else {
        fprintf(stderr, "successfully created instance %d\n", result);
    }

    emugl::setVulkanVersion(1, 3, 0);
    return true;
}

std::optional<uint32_t> findMemoryType(const VulkanDispatch* ivk, VkPhysicalDevice physicalDevice,
                                       uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    ivk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return std::nullopt;
}

}  // namespace vk_util
}  // namespace vk
}  // namespace gfxstream
