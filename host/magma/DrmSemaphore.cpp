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

#include "host/magma/DrmSemaphore.h"

#include <errno.h>
#include <i915_drm.h>
#include <sys/mman.h>

#include "host-common/HostmemIdMapping.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace magma {

DrmSemaphore::DrmSemaphore(DrmSemaphore&& other)
    : mDevice(other.mDevice),
      mGemHandle(other.mGemHandle) {
    // Clear the GEM handle to indicate the object is in an invalid state.
    other.mGemHandle = 0;
}

DrmSemaphore::~DrmSemaphore() {
    if (mGemHandle == 0) {
        // Moved-from object. Return immediately.
        return;
    }
    drm_syncobj_destroy params{.handle = mGemHandle};
    int result = mDevice.ioctl(DRM_IOCTL_SYNCOBJ_DESTROY, &params);
    if (result) {
        ERR("DRM_IOCTL_SYNCOBJ_DESTROY(%d) failed: %d", mGemHandle, errno);
    }
}

std::unique_ptr<DrmSemaphore> DrmSemaphore::create(DrmDevice& device) {
    drm_syncobj_create create_params{};
    int result = device.ioctl(DRM_IOCTL_SYNCOBJ_CREATE, &create_params);
    if (result) {
        ERR("DRM_IOCTL_SYNCOBJ_CREATE failed: %d", errno);
        return nullptr;
    }

    std::unique_ptr<DrmSemaphore> semaphore(new DrmSemaphore(device));
    semaphore->mGemHandle = create_params.handle;

    INFO("Created DrmSemaphore gem %" PRIu32, semaphore->mGemHandle);
    return semaphore;
}

uint32_t DrmSemaphore::getHandle() { return mGemHandle; }

void DrmSemaphore::signal() {
    drm_syncobj_array params{
        .handles = reinterpret_cast<uintptr_t>(&mGemHandle),
        .count_handles = 1
    };
    int result = mDevice.ioctl(DRM_IOCTL_SYNCOBJ_SIGNAL, &params);
    if (result) {
        ERR("DRM_IOCTL_SYNCOBJ_SIGNAL failed: %d", errno);
    }
}

void DrmSemaphore::reset() {
    drm_syncobj_array params{
        .handles = reinterpret_cast<uintptr_t>(&mGemHandle),
        .count_handles = 1
    };
    int result = mDevice.ioctl(DRM_IOCTL_SYNCOBJ_RESET, &params);
    if (result) {
        ERR("DRM_IOCTL_SYNCOBJ_RESET failed: %d", errno);
    }
}

DrmSemaphore::DrmSemaphore(DrmDevice& device) : mDevice(device) {}

}  // namespace magma
}  // namespace gfxstream
