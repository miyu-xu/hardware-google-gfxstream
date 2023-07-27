/*
* Copyright (C) 2023 The Android Open Source Project
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

#include <qemu_pipe_types_bp.h>
#include <stdint.h>

#ifdef VIRTIO_GPU
bool QemuPipeInit(QEMU_PIPE_HANDLE* handle_out, uint64_t* proc_uid_out) { return false; }
#else

#include <cutils/log.h>
#include "services/service_connector.h"

#include <fidl/fuchsia.hardware.goldfish/cpp/wire.h>

static QEMU_PIPE_HANDLE   sProcDevice = 0;

#define GET_STATUS_SAFE(result, member) \
    ((result).ok() ? ((result)->member) : ZX_OK)

bool QemuPipeInit(QEMU_PIPE_HANDLE* handle_out, uint64_t* proc_uid_out) {
    fidl::ClientEnd<fuchsia_hardware_goldfish::Controller> controller_channel{
        zx::channel(GetConnectToServiceFunction()(QEMU_PIPE_PATH))};
    if (!controller_channel) {
        ALOGE("%s: failed to open " QEMU_PIPE_PATH,
              __FUNCTION__);
        return false;
    }

    fidl::WireSyncClient controller(std::move(controller_channel));
    zx::result pipe_device_ends =
        fidl::CreateEndpoints<fuchsia_hardware_goldfish::PipeDevice>();
    if (pipe_device_ends.is_error()) {
        ALOGE("%s: zx_channel_create failed: %s", __FUNCTION__, pipe_device_ends.status_string());
        return false;
    }

    if (fidl::Status result = controller->OpenSession(std::move(pipe_device_ends->server));
        !result.ok()) {
        ALOGE("%s: failed to open session: %s", __FUNCTION__, result.status_string());
        return false;
    }

    fidl::WireSyncClient device(std::move(pipe_device_ends->client));

    auto pipe_ends =
        fidl::CreateEndpoints<::fuchsia_hardware_goldfish::Pipe>();
    if (!pipe_ends.is_ok()) {
        ALOGE("%s: zx_channel_create failed: %d", __FUNCTION__, pipe_ends.status_value());
        return false;
    }

    fidl::WireSyncClient pipe(std::move(pipe_ends->client));
    device->OpenPipe(std::move(pipe_ends->server));

    zx::vmo vmo;
    {
        auto result = pipe->GetBuffer();
        if (!result.ok() || result->res != ZX_OK) {
            ALOGE("%s: failed to get buffer: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return false;
        }
        vmo = std::move(result->vmo);
    }

    size_t len = strlen("pipe:GLProcessPipe");
    zx_status_t status = vmo.write("pipe:GLProcessPipe", 0, len + 1);
    if (status != ZX_OK) {
        ALOGE("%s: failed write pipe name", __FUNCTION__);
        return false;
    }

    {
        auto result = pipe->Write(len + 1, 0);
        if (!result.ok() || result->res != ZX_OK) {
            ALOGD("%s: connecting to pipe service failed: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return false;
        }
    }

    // Send a confirmation int to the host and get per-process unique ID back
    int32_t confirmInt = 100;
    status = vmo.write(&confirmInt, 0, sizeof(confirmInt));
    if (status != ZX_OK) {
        ALOGE("%s: failed write confirm int", __FUNCTION__);
        return false;
    }

    {
        auto result = pipe->DoCall(sizeof(confirmInt), 0, sizeof(uint64_t), 0);
        if (!result.ok() || result->res != ZX_OK) {
            ALOGD("%s: failed to get per-process ID: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return false;
        }
    }

    status = vmo.read(proc_uid_out, 0, sizeof(uint64_t));
    if (status != ZX_OK) {
        ALOGE("%s: failed read per-process ID: %d", __FUNCTION__, status);
        return false;
    }

    sProcDevice = device.TakeClientEnd().TakeChannel().release();

    *handle_out = pipe.TakeClientEnd().TakeChannel().release();

    return true;
}

#endif // VIRTIO_GPU
