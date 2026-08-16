// Copyright (C) 2022 The Android Open Source Project
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

#include "DisplayGl.h"

#if defined(__APPLE__)
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "DisplaySurfaceGl.h"
#include "OpenGLESDispatch/DispatchTables.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "TextureDraw.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace gl {
namespace {

std::shared_future<void> getCompletedFuture() {
    std::shared_future<void> completedFuture =
        std::async(std::launch::deferred, [] {}).share();
    completedFuture.wait();
    return completedFuture;
}

#if defined(__APPLE__)
class FrameMetricsReporter {
   public:
    FrameMetricsReporter()
        : mPath(getenv("HD_FRAME_METRICS_PATH") ? getenv("HD_FRAME_METRICS_PATH") : ""),
          mGeneration(parseGeneration()),
          mWindowStarted(std::chrono::steady_clock::now()) {}

    void reportPresented() {
        if (mPath.empty() || mGeneration == 0) {
            return;
        }
        std::lock_guard<std::mutex> lock(mMutex);
        ++mPresentedFrames;
        ++mWindowFrames;
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - mWindowStarted).count();
        if (elapsed < 500) {
            return;
        }
        const uint64_t fpsMilli64 =
            (mWindowFrames * 1'000'000ULL) / static_cast<uint64_t>(elapsed);
        const uint32_t fpsMilli = static_cast<uint32_t>(
            std::min<uint64_t>(fpsMilli64, std::numeric_limits<uint32_t>::max()));
        publish(fpsMilli);
        mWindowFrames = 0;
        mWindowStarted = now;
    }

   private:
    static uint64_t parseGeneration() {
        const char* value = getenv("HD_FRAME_GENERATION");
        if (!value || value[0] == '\0') {
            return 0;
        }
        char* end = nullptr;
        const unsigned long long parsed = strtoull(value, &end, 10);
        return end != value && end && end[0] == '\0' ? static_cast<uint64_t>(parsed) : 0;
    }

    void publish(uint32_t fpsMilli) const {
        const std::string temporary = mPath + ".tmp." + std::to_string(getpid());
        const int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW,
                            S_IRUSR | S_IWUSR);
        if (fd < 0) {
            return;
        }
        char json[512];
        const int length = snprintf(
            json, sizeof(json),
            "{\"generation\":%llu,\"produced_frames\":%llu,\"imported_frames\":%llu,"
            "\"presented_frames\":%llu,\"dropped_frames\":0,\"cpu_readback_bytes\":0,"
            "\"software_blit_count\":0,\"fps_milli\":%u}\n",
            static_cast<unsigned long long>(mGeneration),
            static_cast<unsigned long long>(mPresentedFrames),
            static_cast<unsigned long long>(mPresentedFrames),
            static_cast<unsigned long long>(mPresentedFrames), fpsMilli);
        bool complete = length > 0 && static_cast<size_t>(length) < sizeof(json);
        size_t written = 0;
        while (complete && written < static_cast<size_t>(length)) {
            const ssize_t count =
                write(fd, json + written, static_cast<size_t>(length) - written);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                complete = false;
                break;
            }
            written += static_cast<size_t>(count);
        }
        close(fd);
        if (complete) {
            (void)rename(temporary.c_str(), mPath.c_str());
        } else {
            (void)unlink(temporary.c_str());
        }
    }

    const std::string mPath;
    const uint64_t mGeneration;
    mutable std::mutex mMutex;
    uint64_t mPresentedFrames = 0;
    uint64_t mWindowFrames = 0;
    std::chrono::steady_clock::time_point mWindowStarted;
};

FrameMetricsReporter& frameMetricsReporter() {
    static FrameMetricsReporter reporter;
    return reporter;
}
#endif

}  // namespace

std::shared_future<void> DisplayGl::post(const Post& post) {
    const auto* surface = getBoundSurface();
    if (!surface) {
        return getCompletedFuture();
    }
    if (post.layers.empty()) {
        clear();
        return getCompletedFuture();
    }
    const auto* surfaceGl = static_cast<const DisplaySurfaceGl*>(surface->getImpl());

    bool hasDrawLayer = false;
    for (const PostLayer& layer : post.layers) {
        if (layer.layerOptions) {
            if (!hasDrawLayer) {
                mTextureDraw->prepareForDrawLayer();
                hasDrawLayer = true;
            }
            layer.colorBuffer->glOpPostLayer(*layer.layerOptions, post.frameWidth,
                                             post.frameHeight);
        } else if (layer.overlayOptions) {
            if (hasDrawLayer) {
                ERR("Cannot mix colorBuffer.postLayer with postWithOverlay!");
            }
            layer.colorBuffer->glOpPostViewportScaledWithOverlay(
                layer.overlayOptions->rotation, layer.overlayOptions->dx, layer.overlayOptions->dy);
        }
    }
    if (hasDrawLayer) {
        mTextureDraw->cleanupForDrawLayer();
    }

#if defined(__APPLE__)
    if (s_egl.eglSwapBuffers(surfaceGl->mDisplay, surfaceGl->mSurface) == EGL_TRUE) {
        frameMetricsReporter().reportPresented();
    }
#else
    s_egl.eglSwapBuffers(surfaceGl->mDisplay, surfaceGl->mSurface);
#endif

    return getCompletedFuture();
}

void DisplayGl::viewport(int width, int height) {
    mViewportWidth = width;
    mViewportHeight = height;
    s_gles2.glViewport(0, 0, mViewportWidth, mViewportHeight);
}

void DisplayGl::clear() {
    const auto* surface = getBoundSurface();
    if (!surface) {
        return;
    }
    const auto* surfaceGl = static_cast<const DisplaySurfaceGl*>(surface->getImpl());
#ifndef __linux__
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    s_egl.eglSwapBuffers(surfaceGl->mDisplay, surfaceGl->mSurface);
#endif
}

}  // namespace gl
}  // namespace gfxstream
