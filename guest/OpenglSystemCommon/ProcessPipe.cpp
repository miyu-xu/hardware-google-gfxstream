/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "ProcessPipe.h"
#include "HostConnection.h"
#include "renderControl_enc.h"

#include <qemu_pipe_bp.h>

#if PLATFORM_SDK_VERSION < 26
#include <cutils/log.h>
#else
#include <log/log.h>
#endif
#include <pthread.h>
#include <errno.h>

#ifdef __Fuchsia__
#include <zircon/syscalls.h>
#endif

#include "VirtioGpuPipeStream.h"
static VirtioGpuPipeStream* sVirtioGpuPipeStream = 0;
static int sStreamHandle = -1;

static QEMU_PIPE_HANDLE   sProcPipe = 0;
static pthread_once_t     sProcPipeOnce = PTHREAD_ONCE_INIT;
// sProcUID is a unique ID per process assigned by the host.
// It is different from getpid().
static uint64_t           sProcUID = 0;
static volatile HostConnectionType sConnType = HOST_CONNECTION_VIRTIO_GPU_PIPE;

static uint32_t* sSeqnoPtr = 0;

// Meant to be called only once per process.
static void initSeqno() {
    // So why do we reinitialize here? It's for testing purposes only;
    // we have a unit test that exercise the case where this sequence
    // number is reset as a result of guest process kill.
    if (sSeqnoPtr) delete sSeqnoPtr;
    sSeqnoPtr = new uint32_t;
    *sSeqnoPtr = 0;
}

// processPipeInitOnce is used to generate a process unique ID (puid).
// processPipeInitOnce will only be called at most once per process.
// Use it with pthread_once for thread safety.
// The host associates resources with process unique ID (puid) for memory cleanup.
// It will fallback to the default path if the host does not support it.
// Processes are identified by acquiring a per-process 64bit unique ID from the
// host.
#ifdef __Fuchsia__
extern bool QemuPipeInit(QEMU_PIPE_HANDLE* handle_out, uint64_t* proc_uid_out);
#else
extern bool QemuPipeInit(QEMU_PIPE_HANDLE* handle_out, uint64_t* proc_uid_out) {
    QEMU_PIPE_HANDLE pipe_handle = qemu_pipe_open("GLProcessPipe");
    if (!qemu_pipe_valid(pipe_handle)) {
        ALOGW("Process pipe failed");
        return false;
    }
    // Send a confirmation int to the host
    int32_t confirmInt = 100;
    if (qemu_pipe_write_fully(pipe_handle, &confirmInt, sizeof(confirmInt))) { // failed
        qemu_pipe_close(pipe_handle);
        ALOGW("Process pipe failed");
        return false;
    }

    // Ask the host for per-process unique ID
    if (qemu_pipe_read_fully(pipe_handle, proc_uid_out, sizeof(uint64_t))) {
        qemu_pipe_close(pipe_handle);
        *proc_uid_out = 0;
        ALOGW("Process pipe failed");
        return false;
    }
    return true;
}
#endif // Fuchsia

static void processPipeInitOnce() {
    initSeqno();

#if defined(HOST_BUILD) || !defined(GFXSTREAM)
    QemuPipeInit(&sProcPipe, &sProcUID);
#else // HOST_BUILD
    switch (sConnType) {
        // TODO: Move those over too
        case HOST_CONNECTION_QEMU_PIPE:
        case HOST_CONNECTION_ADDRESS_SPACE:
        case HOST_CONNECTION_TCP:
            QemuPipeInit(&sProcPipe, &sProcUID);
            break;
        case HOST_CONNECTION_VIRTIO_GPU_PIPE:
        case HOST_CONNECTION_VIRTIO_GPU_ADDRESS_SPACE: {
            sVirtioGpuPipeStream = new VirtioGpuPipeStream(4096, sStreamHandle);
            sProcUID = sVirtioGpuPipeStream->initProcessPipe();
            break;
        }
    }
#endif // !HOST_BUILD
}

bool processPipeInit(int streamHandle, HostConnectionType connType, renderControl_encoder_context_t *rcEnc) {
    sConnType = connType;
#ifndef __Fuchsia__
    sStreamHandle = streamHandle;
#endif // !__Fuchsia
    pthread_once(&sProcPipeOnce, processPipeInitOnce);
    bool pipeHandleInvalid = !sProcPipe;
#ifndef __Fuchsia__
    pipeHandleInvalid = pipeHandleInvalid && !sVirtioGpuPipeStream;
#endif // !__Fuchsia__
    if (pipeHandleInvalid) return false;
    rcEnc->rcSetPuid(rcEnc, sProcUID);
    return true;
}

uint64_t getPuid() {
    return sProcUID;
}

void processPipeRestart() {
    ALOGW("%s: restarting process pipe\n", __func__);
    bool isPipe = false;

    switch (sConnType) {
        // TODO: Move those over too
        case HOST_CONNECTION_QEMU_PIPE:
        case HOST_CONNECTION_ADDRESS_SPACE:
        case HOST_CONNECTION_TCP:
            isPipe = true;
            break;
        case HOST_CONNECTION_VIRTIO_GPU_PIPE:
        case HOST_CONNECTION_VIRTIO_GPU_ADDRESS_SPACE: {
            isPipe = false;
            break;
        }
    }

    sProcUID = 0;

#ifdef __Fuchsia__
    zx_handle_close(sProcPipe);
    sProcPipe = ZX_HANDLE_INVALID;
#else
    if (isPipe) {
        if (qemu_pipe_valid(sProcPipe)) {
            qemu_pipe_close(sProcPipe);
            sProcPipe = 0;
        }
    } else {
        delete sVirtioGpuPipeStream;
        sVirtioGpuPipeStream = nullptr;
    }
#endif // __Fuchsia__

    processPipeInitOnce();
}

void refreshHostConnection() {
    HostConnection* hostConn = HostConnection::get();
    ExtendedRCEncoderContext* rcEnc = hostConn->rcEncoder();
    rcEnc->rcSetPuid(rcEnc, sProcUID);
}

uint32_t* getSeqnoPtrForProcess() {
    // It's assumed process pipe state has already been initialized.
    return sSeqnoPtr;
}
