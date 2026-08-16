// Copyright 2026 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstdint>
#include <memory>

namespace gfxstream {

// Instance-scoped bridge from the posted gfxstream color buffers to the HD frame broker. The
// implementation is Windows-only because its wire handles are duplicated Win32 Vulkan external
// memory handles. No pixel data is mapped or copied by this bridge.
class HdFrameBridgeWin32 {
   public:
    static std::unique_ptr<HdFrameBridgeWin32> createFromEnvironment();

    virtual ~HdFrameBridgeWin32() = default;

    virtual bool publish(uint32_t resourceHandle, uint32_t width, uint32_t height,
                         uint32_t virglFormat) = 0;
};

}  // namespace gfxstream
