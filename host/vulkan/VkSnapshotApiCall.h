// Copyright (C) 2024 The Android Open Source Project
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

#pragma once

#include <vector>

#include "aemu/base/containers/EntityManager.h"

namespace gfxstream {
namespace vk {

using VkSnapshotApiCallHandle = uint64_t;

struct VkSnapshotApiCallInfo {
    VkSnapshotApiCallHandle handle = -1;

    // Raw packet from VkDecoder.
    std::vector<uint8_t> packet;

    // Book-keeping for which handles were created by this API
    std::vector<uint64_t> createdHandles;

    void addOrderedBoxedHandlesCreatedByCall(const uint64_t* boxedHandles,
                                             uint32_t boxedHandlesCount) {
        createdHandles.insert(createdHandles.end(), boxedHandles, boxedHandles + boxedHandlesCount);
    }
};

using VkSnapshotApiCallManager = android::base::EntityManager<32, 16, 16, VkSnapshotApiCallInfo>;

}  // namespace vk
}  // namespace gfxstream