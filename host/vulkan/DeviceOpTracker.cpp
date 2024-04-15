// Copyright 2018 The Android Open Source Project
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

#include "DeviceOpTracker.h"

#include <type_traits>

#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace vk {
namespace {

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;

template <typename T>
inline constexpr bool always_false_v = false;

}  // namespace

DeviceOpTracker::DeviceOpTracker(VkDevice device, VulkanDispatch* deviceDispatch)
    : mDevice(device), mDeviceDispatch(deviceDispatch) {}

void DeviceOpTracker::AddPendingGarbage(DeviceOpWaitable waitable, VkFence fence) {
    std::lock_guard<std::mutex> lock(mPendingGarbageMutex);
    mPendingGarbage.emplace_back(waitable, fence);
}

void DeviceOpTracker::AddPendingGarbage(DeviceOpWaitable waitable, VkSemaphore semaphore) {
    std::lock_guard<std::mutex> lock(mPendingGarbageMutex);
    mPendingGarbage.emplace_back(waitable, semaphore);
}

void DeviceOpTracker::PollAndProcessGarbage() {
    {
        std::lock_guard<std::mutex> lock(mMutex);

        for (auto it = mPollFunctions.begin(); it != mPollFunctions.end();) {
            DeviceOpStatus status = (*it)();
            if (status == DeviceOpStatus::kDone) {
                it = mPollFunctions.erase(it);
            } else if (status == DeviceOpStatus::kFailure) {
                it = mPollFunctions.erase(it);
            } else {
                break;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mPendingGarbageMutex);

        for (auto it = mPendingGarbage.begin(); it != mPendingGarbage.end();) {
            const bool done = IsDone(it->waitable);
            if (!done) {
                break;
            }

            std::visit(
                [this](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, VkFence>) {
                        mDeviceDispatch->vkDestroyFence(mDevice, arg, nullptr);
                    } else if constexpr (std::is_same_v<T, VkSemaphore>) {
                        mDeviceDispatch->vkDestroySemaphore(mDevice, arg, nullptr);
                    } else {
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                    }
                },
                it->obj);

            it = mPendingGarbage.erase(it);
        }
    }
}

void DeviceOpTracker::AddPendingDeviceOp(std::function<DeviceOpStatus()> pollFunction) {
    std::lock_guard<std::mutex> lock(mMutex);

    mPollFunctions.push_back(std::move(pollFunction));
}

DeviceOpBuilder::DeviceOpBuilder(DeviceOpTracker& tracker) : mTracker(tracker) {}

DeviceOpBuilder::~DeviceOpBuilder() {
    if (!mSubmittedFence) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Invalid usage: failed to call OnQueueSubmittedWithFence().";
    }
}

VkFence DeviceOpBuilder::CreateFenceForOp() {
    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    VkFence fence = VK_NULL_HANDLE;
    VkResult result = mTracker.mDeviceDispatch->vkCreateFence(mTracker.mDevice, &fenceCreateInfo,
                                                              nullptr, &fence);

    mCreatedFence = fence;
    if (result != VK_SUCCESS) {
        ERR("DeviceOpBuilder failed to create VkFence!");
        return VK_NULL_HANDLE;
    }
    return fence;
}

DeviceOpWaitable DeviceOpBuilder::OnQueueSubmittedWithFence(VkFence fence) {
    if (mCreatedFence.has_value() && fence != mCreatedFence) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Invalid usage: failed to call OnQueueSubmittedWithFence() with the fence "
            << "requested from CreateFenceForOp.";
    }
    mSubmittedFence = fence;

    const bool destroyFenceOnCompletion = mCreatedFence.has_value();

    std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
    DeviceOpWaitable future = promise->get_future().share();

    mTracker.AddPendingDeviceOp([device = mTracker.mDevice,
                                 deviceDispatch = mTracker.mDeviceDispatch, fence,
                                 promise = std::move(promise), destroyFenceOnCompletion] {
        if (fence == VK_NULL_HANDLE) {
            return DeviceOpStatus::kDone;
        }

        VkResult result =
            deviceDispatch->vkWaitForFences(device, 1, &fence, /*waitAll=*/VK_TRUE, /*timeout=*/0);
        if (result == VK_TIMEOUT) {
            return DeviceOpStatus::kPending;
        }

        if (destroyFenceOnCompletion) {
            deviceDispatch->vkDestroyFence(device, fence, nullptr);
        }
        promise->set_value();

        return result == VK_SUCCESS ? DeviceOpStatus::kDone : DeviceOpStatus::kFailure;
    });

    return future;
}

}  // namespace vk
}  // namespace gfxstream