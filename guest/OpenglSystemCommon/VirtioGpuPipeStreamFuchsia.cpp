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

#include "VirtioGpuPipeStream.h"

#include <cutils/log.h>

VirtioGpuPipeStream::VirtioGpuPipeStream(size_t bufsize) : gfxstream::IOStream(bufsize) {}

VirtioGpuPipeStream::VirtioGpuPipeStream(size_t bufsize, int stream_handle) : VirtioGpuPipeStream(bufsize) {}

VirtioGpuPipeStream::~VirtioGpuPipeStream() {}

int VirtioGpuPipeStream::connect(const char* serviceName) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return -1;
}

// static
int VirtioGpuPipeStream::openRendernode() {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return -1;
}

uint64_t VirtioGpuPipeStream::initProcessPipe() {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return 0;
}

void *VirtioGpuPipeStream::allocBuffer(size_t minSize) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return nullptr;
}

int VirtioGpuPipeStream::commitBuffer(size_t size) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return 0;
}

const unsigned char *VirtioGpuPipeStream::readFully( void *buf, size_t len) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return nullptr;
}

const unsigned char *VirtioGpuPipeStream::commitBufferAndReadFully(
        size_t size, void *buf, size_t len) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return nullptr;
}

const unsigned char *VirtioGpuPipeStream::read( void *buf, size_t *inout_len) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return nullptr;
}

int VirtioGpuPipeStream::recv(void *buf, size_t len) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return 0;
}

int VirtioGpuPipeStream::writeFully(const void *buf, size_t len) {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return 0;
}

int VirtioGpuPipeStream::getSocket() const {
    ALOGE("%s: unimplemented", __FUNCTION__);
    return 0;
}
