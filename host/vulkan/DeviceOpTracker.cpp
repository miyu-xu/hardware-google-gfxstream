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

#include "DeviceOpTracker.h"

#include <type_traits>

#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace vk {
namespace {

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;

constexpr const size_t kSizeLoggingThreshold = 20;

constexpr const auto kTimeThreshold = std::chrono::seconds(5);

template <typename T>
inline constexpr bool always_false_v = false;

}  // namespace

DeviceOpTracker::DeviceOpTracker(VkDevice device, VulkanDispatch* deviceDispatch)
    : mDevice(device), mDeviceDispatch(deviceDispatch) {}

void DeviceOpTracker::AddPendingGarbage(DeviceOpWaitable waitable, VkFence fence) {
    std::lock_guard<std::mutex> lock(mPendingGarbageMutex);

    mPendingGarbage.push_back(PendingGarabage{
        .waitable = std::move(waitable),
        .obj = fence,
        .timepoint = std::chrono::system_clock::now(),
    });

    if (mPendingGarbage.size() > kSizeLoggingThreshold) {
        WARN("VkDevice:%p has %d pending garbage objects.", mDevice, mPendingGarbage.size());
    }
}

void DeviceOpTracker::AddPendingGarbage(DeviceOpWaitable waitable, VkSemaphore semaphore) {
    std::lock_guard<std::mutex> lock(mPendingGarbageMutex);

    mPendingGarbage.push_back(PendingGarabage{
        .waitable = std::move(waitable),
        .obj = semaphore,
        .timepoint = std::chrono::system_clock::now(),
    });

    if (mPendingGarbage.size() > kSizeLoggingThreshold) {
        WARN("VkDevice:%p has %d pending garbage objects.", mDevice, mPendingGarbage.size());
    }
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

        if (mPollFunctions.size() > kSizeLoggingThreshold) {
            WARN("VkDevice:%p has %d pending waitables.", mDevice, mPollFunctions.size());
        }
    }

    const auto now = std::chrono::system_clock::now();
    const auto old = now - kTimeThreshold;
    {
        std::lock_guard<std::mutex> lock(mPendingGarbageMutex);

        for (auto garbageIt = mPendingGarbage.begin(); garbageIt != mPendingGarbage.end();) {
            PendingGarabage& garbage = *garbageIt;

            if (garbage.timepoint < old) {
                const auto difference =
                    std::chrono::duration_cast<std::chrono::milliseconds>(garbage.timepoint - now);
                WARN("VkDevice:%p had a waitable pending for %d milliseconds.", mDevice,
                     difference.count());
            } else {
                const bool done = IsDone(garbage.waitable);
                if (!done) {
                    break;
                }
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
                garbage.obj);

            garbageIt = mPendingGarbage.erase(garbageIt);
        }

        if (mPendingGarbage.size() > kSizeLoggingThreshold) {
            WARN("VkDevice:%p has %d pending garbage objects.", mDevice, mPendingGarbage.size());
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