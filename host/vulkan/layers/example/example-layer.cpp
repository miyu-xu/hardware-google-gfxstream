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

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

#define LOG(...) fprintf(stderr, ">>> [Example Vulkan Layer] " __VA_ARGS__)

// #define DUMP_GETINSTANCEPROCADDR 1
#if DUMP_GETINSTANCEPROCADDR
#define D(...) LOG(__VA_ARGS__)
#else
#define D(...) void(0)
#endif  // DUMP_GETINSTACEPROCADDR

namespace {

typedef struct InstanceInfo {
    PFN_vkGetInstanceProcAddr nextGetInstanceProcAddr = nullptr;
} InstanceInfo;

class InstanceMap {
public:
    InstanceMap() = default;
    void addEntry(VkInstance instance, InstanceInfo info) {
        std::scoped_lock lock(mMutex);
        mMap[instance] = info;
    }

    void removeEntry(VkInstance instance) {
        std::scoped_lock lock(mMutex);
        mMap.erase(instance);
    }

    using EntryFunc = std::function<void(VkInstance, const InstanceInfo*)>;
    void useEntryWithCallback(VkInstance instance, EntryFunc func) {
        std::scoped_lock lock(mMutex);
        if (mMap.find(instance) != mMap.end()) {
            func(instance, &mMap[instance]);
        } else {
            func(instance, nullptr);
        }
    }
private:
    std::mutex mMutex;
    std::unordered_map<VkInstance, InstanceInfo> mMap;
};

InstanceMap sInstanceMap;

VkLayerInstanceCreateInfo* get_chain_info(const VkInstanceCreateInfo *pCreateInfo, VkLayerFunction func) {
    VkLayerInstanceCreateInfo *chain_info = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerInstanceCreateInfo *)chain_info->pNext;
    }
    if (chain_info == nullptr) {
        LOG("Can't find VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO\n");
        exit(-1);
    }
    return chain_info;
}

void createInstanceDispatch(VkInstance instance, PFN_vkGetInstanceProcAddr nextGetInstanceProcAddr) {
        sInstanceMap.addEntry(
            instance,
            InstanceInfo{ .nextGetInstanceProcAddr = nextGetInstanceProcAddr, });
}

}  // namespace

// This is the layer's entry point.
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {

    LOG("Entering our vkCreateInstance\n");

    // Get next layer's function pointer for vkGetInstanceProcAddr and vkCreateInstance, stored in
    // pCreateInfo (sType = VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO).
    VkLayerInstanceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    if (chain_info->u.pLayerInfo == nullptr) {
        LOG("ERROR: No next layer!!\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Next layer's vkGetInstanceProcAddr
    PFN_vkGetInstanceProcAddr nextGetInstanceProcAddr =
            chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    if (nextGetInstanceProcAddr == nullptr) {
        LOG("Next layer didn't give their vkGetInstanceProcAddr!!\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Next layer's vkCreateInstance
    PFN_vkCreateInstance nextCreateInstance =
            (PFN_vkCreateInstance)nextGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (nextCreateInstance == NULL) {
        LOG("ERROR: missing vkCreateInstance call for next in chain\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Before calling the next layer's vkCreateInstance, we need to point the VkLayerInstanceLink to
    // the next layer's VkLayerInstanceLink.
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // Call next layer's vkCreateInstance and create the dispatch table
    VkResult result = nextCreateInstance(pCreateInfo, pAllocator, pInstance);
    LOG("Called next layer's vkCreateInstance instance=0x%p result=%d\n", *pInstance, result);
    if(result == VK_SUCCESS) {
        createInstanceDispatch(*pInstance, nextGetInstanceProcAddr);
    }

    return result;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance,
                                                               const char *pName) {
    // instance chain functions we intercept
    if (!strcmp(pName, "vkCreateInstance")) {
        return (PFN_vkVoidFunction)&vkCreateInstance;
    }

    // For all other instance functions, we simply hand it off to the next layer via calling it's
    // vkGetInstaceProcAddr.
    PFN_vkVoidFunction result = nullptr;
    sInstanceMap.useEntryWithCallback(instance, [&](VkInstance instance, const InstanceInfo* info) {
        result = info->nextGetInstanceProcAddr(instance, pName);
        D("Calling next layer's vkGetInstanceProcAddr VkInstance:0x%p pName=%s result_func=0x%p\n",
          instance, pName, result);
    });
    return result;
}