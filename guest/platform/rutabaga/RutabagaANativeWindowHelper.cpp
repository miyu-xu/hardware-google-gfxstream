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

#include "RutabagaANativeWindowHelper.h"

#include <log/log.h>

namespace gfxstream {

RutabagaANativeWindow::RutabagaANativeWindow(
        uint32_t width,
        uint32_t height,
        uint32_t format,
        std::vector<std::unique_ptr<RutabagaAHardwareBuffer>> buffers)
    : mWidth(width),
      mHeight(height),
      mFormat(format),
      mBuffers(std::move(buffers)) {
    for (auto& buffer : mBuffers) {
        mBufferQueue.push_back(QueuedAHB{
            .ahb = buffer.get(),
            .fence = -1,
        });
    }
}

EGLNativeWindowType RutabagaANativeWindow::asEglNativeWindowType() {
    return reinterpret_cast<EGLNativeWindowType>(this);
}

uint32_t RutabagaANativeWindow::getWidth() const {
    return mWidth;
}

uint32_t RutabagaANativeWindow::getHeight() const {
    return mHeight;
}

int RutabagaANativeWindow::getFormat() const {
    return mFormat;
}

int RutabagaANativeWindow::queueBuffer(EGLClientBuffer buffer, int fence) {
    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);

    mBufferQueue.push_back(QueuedAHB{
        .ahb = ahb,
        .fence = fence,
    });

    return 0;
}

int RutabagaANativeWindow::dequeueBuffer(EGLClientBuffer* buffer, int* fence) {
    auto queuedAhb = mBufferQueue.front();
    mBufferQueue.pop_front();

    *buffer = queuedAhb.ahb->asEglClientBuffer();
    *fence = queuedAhb.fence;
    return 0;
}

int RutabagaANativeWindow::cancelBuffer(EGLClientBuffer buffer) {
    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);

    mBufferQueue.push_back(QueuedAHB{
        .ahb = ahb,
        .fence = -1,
    });

    return 0;
}

bool RutabagaANativeWindowHelper::isValid(EGLNativeWindowType window) {
    // TODO: maybe a registry of valid RutabagaANativeWindow-s?
    (void)window;
    return true;
}

bool RutabagaANativeWindowHelper::isValid(EGLClientBuffer buffer) {
    // TODO: maybe a registry of valid RutabagaAHardwareBuffer-s?
    (void)buffer;
    return true;
}

void RutabagaANativeWindowHelper::acquire(EGLNativeWindowType window) {
    // TODO: maybe a registry of valid RutabagaANativeWindow-s?
    (void)window;
}

void RutabagaANativeWindowHelper::release(EGLNativeWindowType window) {
    // TODO: maybe a registry of valid RutabagaANativeWindow-s?
    (void)window;
}

void RutabagaANativeWindowHelper::acquire(EGLClientBuffer buffer) {
    // TODO: maybe a registry of valid RutabagaAHardwareBuffer-s?
    (void)buffer;
}

void RutabagaANativeWindowHelper::release(EGLClientBuffer buffer) {
    // TODO: maybe a registry of valid RutabagaAHardwareBuffer-s?
    (void)buffer;
}

int RutabagaANativeWindowHelper::getConsumerUsage(EGLNativeWindowType window, int* usage) {
    (void)window;
    (void)usage;
    return 0;
}
void RutabagaANativeWindowHelper::setUsage(EGLNativeWindowType window, int usage) {
    (void)window;
    (void)usage;
}

int RutabagaANativeWindowHelper::getWidth(EGLNativeWindowType window) {
    auto anw = reinterpret_cast<RutabagaANativeWindow*>(window);
    return anw->getWidth();
}

int RutabagaANativeWindowHelper::getHeight(EGLNativeWindowType window) {
    auto anw = reinterpret_cast<RutabagaANativeWindow*>(window);
    return anw->getHeight();
}

int RutabagaANativeWindowHelper::getWidth(EGLClientBuffer buffer) {
    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);
    return ahb->getWidth();
}

int RutabagaANativeWindowHelper::getHeight(EGLClientBuffer buffer) {
    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);
    return ahb->getHeight();
}

int RutabagaANativeWindowHelper::getFormat(EGLClientBuffer buffer, Gralloc* helper) {
    (void)helper;

    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);
    return ahb->getAndroidFormat();
}

void RutabagaANativeWindowHelper::setSwapInterval(EGLNativeWindowType window, int interval) {
    ALOGE("Unimplemented");
    (void)window;
    (void)interval;
}

int RutabagaANativeWindowHelper::queueBuffer(EGLNativeWindowType window, EGLClientBuffer buffer, int fence) {
    auto anw = reinterpret_cast<RutabagaANativeWindow*>(window);
    return anw->queueBuffer(buffer, fence);
}

int RutabagaANativeWindowHelper::dequeueBuffer(EGLNativeWindowType window, EGLClientBuffer* buffer, int* fence) {
    auto anw = reinterpret_cast<RutabagaANativeWindow*>(window);
    return anw->dequeueBuffer(buffer, fence);
}

int RutabagaANativeWindowHelper::cancelBuffer(EGLNativeWindowType window, EGLClientBuffer buffer) {
    auto anw = reinterpret_cast<RutabagaANativeWindow*>(window);
    return anw->cancelBuffer(buffer);
}

int RutabagaANativeWindowHelper::getHostHandle(EGLClientBuffer buffer, Gralloc*) {
    auto ahb = reinterpret_cast<RutabagaAHardwareBuffer*>(buffer);
    return ahb->getResourceId();
}

}  // namespace gfxstream
