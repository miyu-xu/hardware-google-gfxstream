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

#include "host/magma/DrmBuffer.h"

#include <errno.h>
#include <i915_drm.h>
#include <sys/mman.h>

#include "host-common/logging.h"
#include "host/BlobManager.h"

namespace gfxstream {
namespace magma {

std::atomic_uint64_t DrmBuffer::mIdNext = 1000001;

DrmBuffer::DrmBuffer(DrmBuffer&& other)
    : mDevice(other.mDevice),
      mGemHandle(other.mGemHandle),
      mSize(other.mSize),
      mHva(other.mHva),
      mId(other.mId) {
    // Clear the GEM handle to indicate the object is in an invalid state.
    other.mGemHandle = 0;
}

DrmBuffer::~DrmBuffer() {
    if (mGemHandle == 0) {
        // Moved-from object. Return immediately.
        return;
    }
    BlobManager::get()->removeMapping(mContextId, mId);
    BlobManager::get()->removeDescriptorInfo(mContextId, mId);
    drm_gem_close params{.handle = mGemHandle};
    int result = mDevice.ioctl(DRM_IOCTL_GEM_CLOSE, &params);
    if (result) {
        ERR("DRM_IOCTL_GEM_CLOSE(%d) failed: %d", mGemHandle, errno);
    }
}

std::unique_ptr<DrmBuffer> DrmBuffer::create(DrmDevice& device, uint32_t context_id,
                                             uint64_t size) {
    // Create a new GEM buffer.
    drm_i915_gem_create create_params{.size = size};
    int result = device.ioctl(DRM_IOCTL_I915_GEM_CREATE, &create_params);
    if (result) {
        ERR("DRM_IOCTL_I915_GEM_CREATE failed: %d", errno);
        return nullptr;
    }

    // Map the buffer.
    drm_i915_gem_mmap mmap_params{.handle = create_params.handle, .size = create_params.size};
    result = device.ioctl(DRM_IOCTL_I915_GEM_MMAP, &mmap_params);
    if (result) {
        ERR("DRM_IOCTL_I915_GEM_MMAP failed: %d", errno);
        return nullptr;
    }

    // Get an fd for the buffer.
    drm_prime_handle handle_params{.handle = create_params.handle};
    result = device.ioctl(DRM_IOCTL_PRIME_HANDLE_TO_FD, &handle_params);
    if (result) {
        ERR("DRM_IOCTL_PRIME_HANDLE_TO_FD failed: %d", errno);
        return nullptr;
    }

    // Create the container to save the various returned handles.
    std::unique_ptr<DrmBuffer> buffer(new DrmBuffer(device));
    buffer->mContextId = context_id;
    buffer->mSize = create_params.size;  // Parameter adjusted by ioctl.
    buffer->mGemHandle = create_params.handle;
    buffer->mHva = reinterpret_cast<void*>(mmap_params.addr_ptr);

    // Assign the next free ID.
    buffer->mId = mIdNext.fetch_add(1);

    // Add the descriptor entry.
    BlobManager::get()->addDescriptorInfo(buffer->mContextId, buffer->mId,
                                          handle_params.fd,  // Container takes ownership.
                                          STREAM_MEM_HANDLE_TYPE_DMABUF, MAP_CACHE_CACHED,
                                          std::nullopt);

    // Add the mapping entry.
    BlobManager::get()->addMapping(buffer->mContextId, buffer->mId, buffer->mHva, MAP_CACHE_CACHED);

    INFO("Created DrmBuffer addr %p size %" PRIu64 " gem %" PRIu32 " fd %" PRId32 " mid %" PRIu64,
         buffer->mHva, buffer->mSize, buffer->mGemHandle, handle_params.fd, buffer->mId);

    return buffer;
}

uint32_t DrmBuffer::getHandle() { return mGemHandle; }

uint64_t DrmBuffer::size() { return mSize; }

void* DrmBuffer::map() { return mHva; }

uint64_t DrmBuffer::getId() { return mId; }

DrmBuffer::DrmBuffer(DrmDevice& device) : mDevice(device) {}

}  // namespace magma
}  // namespace gfxstream
