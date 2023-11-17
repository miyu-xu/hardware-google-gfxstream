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

#include "FuchsiaVirtGpu.h"
#include "virtgpu_drm.h"
#include "virtgpu_gfxstream_protocol.h"

#include "services/service_connector.h"

#include <fidl/fuchsia.gpu.virtio/cpp/wire.h>
#include <cutils/log.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#define PARAM(x) \
    (struct VirtGpuParam) { x, #x, 0 }

// See virgl_hw.h and p_defines.h
#define VIRGL_FORMAT_R8_UNORM 64
#define VIRGL_BIND_CUSTOM (1 << 17)
#define PIPE_BUFFER 0

FuchsiaVirtGpuDevice::FuchsiaVirtGpuDevice(enum VirtGpuCapset capset) : VirtGpuDevice(capset) {
    struct VirtGpuParam params[] = {
        PARAM(VIRTGPU_PARAM_3D_FEATURES),          PARAM(VIRTGPU_PARAM_CAPSET_QUERY_FIX),
        PARAM(VIRTGPU_PARAM_RESOURCE_BLOB),        PARAM(VIRTGPU_PARAM_HOST_VISIBLE),
        PARAM(VIRTGPU_PARAM_CROSS_DEVICE),         PARAM(VIRTGPU_PARAM_CONTEXT_INIT),
        PARAM(VIRTGPU_PARAM_SUPPORTED_CAPSET_IDs), PARAM(VIRTGPU_PARAM_EXPLICIT_DEBUG_NAME),
        PARAM(VIRTGPU_PARAM_CREATE_GUEST_HANDLE),
    };

    memset(&mCaps, 0, sizeof(struct VirtGpuCaps));

    std::vector<std::string> devices = FuchsiaGetVirtioGpuDevices();
    switch (devices.size()) {
    case 0:
        ALOGE("No virtio gpu devices found");
        return;
    case 1:
        break;
    default:
        ALOGW("Found multiple devices, using the first");
    }

    fidl::ClientEnd<fuchsia_gpu_virtio::VirtioGpu> gpu_client_end{
        zx::channel(GetConnectToServiceFunction()(devices[0].c_str()))};
    if (!gpu_client_end) {
        ALOGE("Failed to open devices %s", devices[0].c_str());
        return;
    }

    mClient = fidl::WireSyncClient<fuchsia_gpu_virtio::VirtioGpu>(std::move(gpu_client_end));

    for (uint32_t i = 0; i < kParamMax; i++) {
        uint64_t query_id = params[i].param;
        ALOGI("*** Calling Query %lu", query_id);

        auto wire_result = mClient->Query(fuchsia_gpu_virtio::wire::QueryId(params[i].param));

        if (wire_result.ok() && wire_result->value()->is_simple_result()) {
            uint64_t result = wire_result->value()->simple_result();
            mCaps.params[i] = result;
            ALOGI("*** params[%u] result %lu", i, result);
        } else {
            ALOGE("Query failed: status %d, expected simple result");
        }
    }

    if (capset == kCapsetGfxStreamVulkan) {
        auto query_id = static_cast<uint64_t>(fuchsia_gpu_virtio::wire::QueryId::kQueryIdCapset);
        query_id |= static_cast<uint64_t>(kCapsetGfxStreamVulkan) << 32;
        constexpr uint16_t kVersion = 0;
        query_id |= static_cast<uint64_t>(kVersion) << 16;

        auto wire_result = mClient->Query(fuchsia_gpu_virtio::wire::QueryId(query_id));

        if (wire_result.ok() && wire_result->value()->is_buffer_result()) {
            zx::vmo capset_info = std::move(wire_result->value()->buffer_result());

            zx_status_t status = capset_info.read(&mCaps.vulkanCapset, /*offset=*/0, sizeof(struct vulkanCapset));
            ALOGI("*** Got capset result, read status %d", status);
        } else {
            ALOGE("Query(%lu) failed: status %d, expected buffer result", query_id, wire_result.status());
        }

        // We always need an ASG blob in some cases, so always define blobAlignment
        if (!mCaps.vulkanCapset.blobAlignment) {
            mCaps.vulkanCapset.blobAlignment = 4096;
        }
    }

    // On Fuchsia, virtio-gpu assumes this configuration.

    // ctx_set_params[0].param = VIRTGPU_CONTEXT_PARAM_NUM_RINGS;
    // ctx_set_params[0].value = 2;
    // init.num_params = 1;

    // if (capset != kCapsetNone) {
    //     ctx_set_params[1].param = VIRTGPU_CONTEXT_PARAM_CAPSET_ID;
    //     ctx_set_params[1].value = static_cast<uint32_t>(capset);
    //     init.num_params++;
    // }

    // init.ctx_set_params = (unsigned long long)&ctx_set_params[0];
    // ret = drmIoctl(mDeviceHandle, DRM_IOCTL_VIRTGPU_CONTEXT_INIT, &init);
    // if (ret) {
    //     ALOGE("DRM_IOCTL_VIRTGPU_CONTEXT_INIT failed with %s, continuing without context...",
    //            strerror(errno));
    // }
    mContextId = 1;
    auto wire_result = mClient->CreateContext(mContextId);
    if (!wire_result.ok()) {
        ALOGE("CreateContext failed: %d", wire_result.status());
    }
}

FuchsiaVirtGpuDevice::~FuchsiaVirtGpuDevice() {
}

int64_t FuchsiaVirtGpuDevice::getDeviceHandle(void) {
    ALOGW("%s: unimplemented", __func__);
    return -1;
}

VirtGpuBlobPtr FuchsiaVirtGpuDevice::createPipeBlob(uint32_t size) {
    ALOGE("%s: unimplemented", __func__);
    return nullptr;
}

VirtGpuBlobPtr FuchsiaVirtGpuDevice::createBlob(const struct VirtGpuCreateBlob& blobCreate) {
    ALOGI("*** createBlob size %lu flags 0x%lx\n", blobCreate.size, blobCreate.flags);

    fidl::Arena allocator;
    auto builder = fuchsia_gpu_virtio::wire::VirtioGpuCreateBlobRequest::Builder(allocator);
    builder.size(blobCreate.size).blob_mem(blobCreate.blobMem).id(blobCreate.blobId).flags(blobCreate.flags);

    auto wire_result = mClient->CreateBlob(builder.Build());
    if (!wire_result.ok()) {
        ALOGE("CreateBlob failed");
        return nullptr;
    }

    auto response = std::move(wire_result.value());
    if (!response.is_ok()) {
        ALOGE("CreateBlob failed");
        return nullptr;
    }

    ALOGI("*** createBlob got bo_handle %u resource_handle %u", response->bo_handle, response->resource_handle);

    return std::make_shared<FuchsiaVirtGpuBlob>(this, response->bo_handle, response->resource_handle,
                                         blobCreate.size);
}

VirtGpuBlobPtr FuchsiaVirtGpuDevice::importBlob(const struct VirtGpuExternalHandle& handle) {
    ALOGE("%s: unimplemented", __func__);
    return nullptr;
}

int FuchsiaVirtGpuDevice::execBuffer(struct VirtGpuExecBuffer& execbuffer, VirtGpuBlobPtr blob) {
    ALOGI("*** execBuffer size %u ring_idx %u flags 0x%x handle %u", 
        execbuffer.command_size, execbuffer.ring_idx, execbuffer.flags, execbuffer.handle);

    uint64_t flags = execbuffer.flags & VirtGpuExecBufferFlags::kRingIdx ? execbuffer.ring_idx : 0;

    auto command_data = fidl::VectorView<uint8_t>::FromExternal(
        static_cast<uint8_t*>(execbuffer.command), execbuffer.command_size);

    mClient->ExecuteImmediateCommands(mContextId, flags, std::move(command_data));

    return 0;
}

struct VirtGpuCaps FuchsiaVirtGpuDevice::getCaps(void) { return mCaps; }

VirtGpuDevice* createPlatformVirtGpuDevice(enum VirtGpuCapset capset, int fd) {
    // We don't handle the VirtioGpuPipeStream case.
    assert(fd == -1);
    return new FuchsiaVirtGpuDevice(capset);
}
