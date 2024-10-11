// Copyright (C) 2024 The Android Open Source Project
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

#include "VirtioGpuRingBlob.h"

namespace gfxstream {
namespace host {

#ifdef GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT

using gfxstream::host::snapshot::VirtioGpuRingBlobSnapshot;

std::optional<VirtioGpuRingBlobSnapshot> SnapshotRingBlob(const RingBlob& resource) {
    VirtioGpuRingBlobSnapshot snapshot;

    if (std::holds_alternative<std::unique_ptr<android::base::SharedMemory>>(resource)) {
        snapshot.set_type(VirtioGpuRingBlobSnapshot::TYPE_SHARED_MEMORY);
    } else {
        snapshot.set_type(VirtioGpuRingBlobSnapshot::TYPE_HOST_MEMORY);
    }

    return snapshot;
}

std::optional<std::unique_ptr<RingBlob>> RestoreRingBlob(
    const VirtioGpuRingBlobSnapshot& snapshot) {
    return std::nullopt;
}

#endif

}  // namespace host
}  // namespace gfxstream