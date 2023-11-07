/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VirtGpu.h"
#include "VirtGpuFuchsia.h"

#include <lib/zx/vmar.h>

#include <cutils/log.h>
#include <unistd.h>

VirtGpuBlob::VirtGpuBlob(VirtGpuDevice* device, uint32_t blobHandle, uint32_t resourceHandle,
                         uint64_t size)
    : mFuchsiaImpl(device->getFuchsiaImpl()),
      mBlobHandle(blobHandle),
      mResourceHandle(resourceHandle),
      mSize(size) {}

VirtGpuBlob::~VirtGpuBlob(void) {}

uint32_t VirtGpuBlob::getBlobHandle(void) {
    return mBlobHandle;
}

uint32_t VirtGpuBlob::getResourceHandle(void) {
    return mResourceHandle;
}

VirtGpuBlobMappingPtr VirtGpuBlob::createMapping(void) {
    zx::vmo vmo;
    {
        auto wire_result = mFuchsiaImpl->virtio_gpu_->GetBlobHandle(mBlobHandle);
        if (!wire_result.ok()) {
            ALOGE("%s: GetBlobHandle failed: %d", __FUNCTION__, wire_result.status());
            return nullptr;
        }
        vmo = std::move(wire_result->value()->vmo);
    }

    zx::vmar vmar;
    zx_vaddr_t virt_addr_;
    if (zx_status_t status = zx::vmar::root_self()->map(ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, /*vmar_offset=*/0,
                                      vmo,
                                      /*vmo_offset=*/0, mSize, &virt_addr_); status != ZX_OK) {
        ALOGE("Map failed: %d", status);
        return nullptr;
    }

    ALOGI("%s: mapped %lu bytes to 0x%lx", __FUNCTION__, mSize, virt_addr_);

    return std::make_shared<VirtGpuBlobMapping>(shared_from_this(), reinterpret_cast<uint8_t*>(virt_addr_), mSize);
}

int VirtGpuBlob::wait() {
    ALOGE("*** %s not (well) implemented", __FUNCTION__);
    sleep(1);
    return 0;
}
