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

#ifdef _WIN32
#include <string>
#endif

namespace gfxstream {

// Starts the optional HD host-recording control endpoint from
// HD_HOST_RECORDER_ENDPOINT. A missing endpoint keeps the recorder completely dormant.
void startHdHostRecorderControl();

// Stops the control endpoint and finalizes an active recording before FrameBuffer teardown.
void stopHdHostRecorderControl();

#ifdef _WIN32
// Runs the production Media Foundation writer against deterministic BGRA frames. The caller owns
// the output path and must remove it after validating the resulting MP4.
__declspec(dllexport) std::string runHdHostRecorderWindowsProbe(const std::string& outputPath);
#endif

}  // namespace gfxstream
