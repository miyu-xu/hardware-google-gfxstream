// Copyright (C) 2020 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <vector>

#include "OSWindow.h"
#include "aemu/base/system/System.h"
#include "host-common/testing/MockGraphicsAgentFactory.h"
#include "virgl_hw.h"
#include "gfxstream/virtio-gpu-gfxstream-renderer-unstable.h"
#include "gfxstream/virtio-gpu-gfxstream-renderer.h"

using android::base::sleepMs;

class GfxStreamBackendTest : public ::testing::Test {
private:
 static void sWriteFence(void* cookie, struct stream_renderer_fence* fence) {
     uint32_t current = *(uint32_t*)cookie;
     if (current < fence->fence_id) *(uint64_t*)(cookie) = fence->fence_id;
 }

protected:
    uint32_t cookie;
    static const bool useWindow;
    std::vector<stream_renderer_param> streamRendererParams;
    std::vector<stream_renderer_param> minimumRequiredParams;
    static constexpr uint32_t width = 256;
    static constexpr uint32_t height = 256;
    static std::unique_ptr<OSWindow> window;
    static constexpr int rendererFlags = STREAM_RENDERER_FLAGS_USE_GLES_BIT;
    static constexpr int surfacelessFlags = STREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT;

    GfxStreamBackendTest()
        : cookie(0),
          streamRendererParams{{STREAM_RENDERER_PARAM_USER_DATA,
                                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&cookie))},
                               {STREAM_RENDERER_PARAM_FENCE_CALLBACK,
                                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&sWriteFence))},
                               {STREAM_RENDERER_PARAM_RENDERER_FLAGS, surfacelessFlags},
                               {STREAM_RENDERER_PARAM_WIN0_WIDTH, width},
                               {STREAM_RENDERER_PARAM_WIN0_HEIGHT, height}},
          minimumRequiredParams{{STREAM_RENDERER_PARAM_USER_DATA,
                                 static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&cookie))},
                                {STREAM_RENDERER_PARAM_FENCE_CALLBACK,
                                 static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&sWriteFence))},
                                {STREAM_RENDERER_PARAM_RENDERER_FLAGS, surfacelessFlags}} {}

    static void SetUpTestSuite() {
        android::emulation::injectGraphicsAgents(android::emulation::MockGraphicsAgentFactory());
        if (useWindow) {
            window.reset(CreateOSWindow());
        }
    }

    static void TearDownTestSuite() { window.reset(nullptr); }

    void SetUp() override {
        android::base::setEnvironmentVariable("ANDROID_GFXSTREAM_EGL", "1");
        if (useWindow) {
            window->initialize("GfxStreamBackendTestWindow", width, height);
            window->setVisible(true);
            window->messageLoop();
        }
    }

    void TearDown() override {
        // Ensure background threads aren't mid-initialization.
        sleepMs(100);
        if (useWindow) {
            window->destroy();
        }
        stream_renderer_teardown();
    }
};

std::unique_ptr<OSWindow> GfxStreamBackendTest::window = nullptr;

const bool GfxStreamBackendTest::useWindow =
        android::base::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_WINDOW") == "1";

TEST_F(GfxStreamBackendTest, Init) {
}

