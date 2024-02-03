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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "RutabagaGralloc.h"

#include <optional>

#include <cutils/log.h>

#include "drm_fourcc.h"

namespace gfxstream {
namespace {

static constexpr int numFds = 0;
static constexpr int numInts = 1;

std::optional<uint32_t> GlFormatToDrmFormat(uint32_t glFormat) {
    switch (glFormat) {
        case kGlRGB:
            return DRM_FORMAT_BGR888;
        case kGlRGB565:
            return DRM_FORMAT_BGR565;
        case kGlRGBA:
            return DRM_FORMAT_ABGR8888;
    }
    return std::nullopt;
}

std::optional<uint32_t> DrmToVirglFormat(uint32_t drmFormat) {
    switch (drmFormat) {
        case DRM_FORMAT_ABGR8888:
            return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DRM_FORMAT_BGR888:
            return VIRGL_FORMAT_R8G8B8_UNORM;
        case DRM_FORMAT_BGR565:
            return VIRGL_FORMAT_B5G6R5_UNORM;
    }
    return std::nullopt;
}

}  // namespace

RutabagaAHardwareBuffer::RutabagaAHardwareBuffer(
        uint32_t width,
        uint32_t height,
        VirtGpuBlobPtr resource)
    : mWidth(width),
      mHeight(height),
      mResource(resource) {
    mHandle = native_handle_create(numFds, numInts);
    mHandle->data[0] = mResource->getResourceHandle();
}

RutabagaAHardwareBuffer::~RutabagaAHardwareBuffer() {
    native_handle_close(mHandle);
}

uint32_t RutabagaAHardwareBuffer::getResourceId() const {
    return mResource->getResourceHandle();
}

uint32_t RutabagaAHardwareBuffer::getWidth() const {
    return mWidth;
}

uint32_t RutabagaAHardwareBuffer::getHeight() const {
    return mHeight;
}

int RutabagaAHardwareBuffer::getAndroidFormat() const {
    return /*AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM=*/1;
}

uint32_t RutabagaAHardwareBuffer::getDrmFormat() const {
    return DRM_FORMAT_ABGR8888;
}

AHardwareBuffer* RutabagaAHardwareBuffer::asAHardwareBuffer() {
    return reinterpret_cast<AHardwareBuffer*>(this);
}

buffer_handle_t RutabagaAHardwareBuffer::asBufferHandle() {
    return reinterpret_cast<buffer_handle_t>(mHandle);
}

EGLClientBuffer RutabagaAHardwareBuffer::asEglClientBuffer() {
    return reinterpret_cast<EGLClientBuffer>(this);
}

RutabagaGralloc::RutabagaGralloc() {}

uint32_t RutabagaGralloc::createColorBuffer(
        void*,
        int width,
        int height,
        uint32_t glFormat) {
    auto drmFormat = GlFormatToDrmFormat(glFormat);
    if (!drmFormat) {
        ALOGE("Unhandled format");
    }

    auto ahb = allocate(width, height, *drmFormat);

    RutabagaAHardwareBuffer* rahb = reinterpret_cast<RutabagaAHardwareBuffer*>(ahb);
    return rahb->getResourceId();
}

int RutabagaGralloc::allocate(
        uint32_t width,
        uint32_t height,
        uint32_t format,
        uint64_t usage,
        AHardwareBuffer** outputAhb) {
    (void)width;
    (void)height;
    (void)format;
    (void)usage;
    (void)outputAhb;

    // TODO: support export flow?
    ALOGE("Unimplemented");

    return 0;
}

AHardwareBuffer* RutabagaGralloc::allocate(
        uint32_t width,
        uint32_t height,
        uint32_t format) {
    ALOGE("Allocating AHB w:%u, h:%u, format %u", width, height, format);

    auto device = VirtGpuDevice::getInstance();
    if (!device) {
        ALOGE("Failed to allocate: no virtio gpu device.");
        return nullptr;
    }

    auto virglFormat = DrmToVirglFormat(format);
    if (!virglFormat) {
        ALOGE("Unhandled DRM format:%u", format);
        return nullptr;
    }

    auto resource = device->createVirglBlob(width, height, *virglFormat);
    if (!resource) {
        ALOGE("Failed to allocate: failed to create virtio resource.");
        return nullptr;
    }

    resource->wait();

    return reinterpret_cast<AHardwareBuffer*>(new RutabagaAHardwareBuffer(width, height, std::move(resource)));
}

void RutabagaGralloc::acquire(AHardwareBuffer* ahb) {
    // TODO
    (void)ahb;
}

void RutabagaGralloc::release(AHardwareBuffer* ahb) {
    // TODO
    (void)ahb;
}

uint32_t RutabagaGralloc::getHostHandle(const native_handle_t* handle) {
    const auto* ahb = reinterpret_cast<const RutabagaAHardwareBuffer*>(handle);
    return ahb->getResourceId();
}

uint32_t RutabagaGralloc::getHostHandle(const AHardwareBuffer* handle) {
    const auto* ahb = reinterpret_cast<const RutabagaAHardwareBuffer*>(handle);
    return ahb->getResourceId();
}

int RutabagaGralloc::getFormat(const native_handle_t* handle) {
    const auto* ahb = reinterpret_cast<const RutabagaAHardwareBuffer*>(handle);
    return ahb->getAndroidFormat();
}

int RutabagaGralloc::getFormat(const AHardwareBuffer* handle) {
    const auto* ahb = reinterpret_cast<const RutabagaAHardwareBuffer*>(handle);
    return ahb->getAndroidFormat();
}

uint32_t RutabagaGralloc::getFormatDrmFourcc(const AHardwareBuffer* handle) {
    const auto* ahb = reinterpret_cast<const RutabagaAHardwareBuffer*>(handle);
    return ahb->getDrmFormat();
}

size_t RutabagaGralloc::getAllocatedSize(const native_handle_t*) {
    ALOGE("Unimplemented.");
    return 0;
}

size_t RutabagaGralloc::getAllocatedSize(const AHardwareBuffer*) {
    ALOGE("Unimplemented.");
    return 0;
}

Gralloc* createPlatformGralloc(int /*deviceFd*/) {
    return new RutabagaGralloc();
}

}  // namespace gfxstream
