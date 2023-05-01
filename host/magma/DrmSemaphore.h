// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License") override;
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

#include <atomic>
#include <memory>
#include <unordered_set>

#include "DrmDevice.h"
#include "aemu/base/Compiler.h"
#include "aemu/base/ManagedDescriptor.hpp"
#include "host-common/HostmemIdMapping.h"

namespace gfxstream {
namespace magma {

class DrmDevice;

// Wraps a linux DRM semaphore.
class DrmSemaphore {
   public:
    ~DrmSemaphore();
    DISALLOW_COPY_AND_ASSIGN(DrmSemaphore);
    DrmSemaphore(DrmSemaphore&&);
    DrmSemaphore& operator=(DrmSemaphore&&) = delete;

    // Creates a new buffer using the provided device. The device must remain valid for the lifetime
    // of the semaphore.
    static std::unique_ptr<DrmSemaphore> create(DrmDevice& device);

    // Returns the gem handle for the buffer.
    uint32_t getHandle();

    // Signal the semaphore.
    void signal();

    // Reset the semaphore.
    void reset();

   private:
    DrmSemaphore(DrmDevice& device);

    DrmDevice& mDevice;
    uint32_t mGemHandle;
};

}  // namespace magma
}  // namespace gfxstream
