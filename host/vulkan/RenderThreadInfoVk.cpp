// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "RenderThreadInfoVk.h"

#include "FrameBuffer.h"
#include "host-common/GfxstreamFatalError.h"

namespace gfxstream {
namespace vk {
using android::base::Stream;

static thread_local RenderThreadInfoVk* tlThreadInfo = nullptr;

RenderThreadInfoVk::RenderThreadInfoVk() {
    if (tlThreadInfo != nullptr) {
        GFXSTREAM_ABORT(emugl::FatalError(emugl::ABORT_REASON_OTHER))
            << "Attempted to set thread local Vk render thread info twice.";
    }
    tlThreadInfo = this;
}

RenderThreadInfoVk::~RenderThreadInfoVk() { tlThreadInfo = nullptr; }

RenderThreadInfoVk* RenderThreadInfoVk::get() { return tlThreadInfo; }

void RenderThreadInfoVk::onSave(Stream* stream) {
    if (isSnapshotCorrupted()) {
        fprintf(stderr, "%s %d corrupted %p\n", __func__, __LINE__, this);
        stream->putByte(1);
    } else {
        stream->putByte(0);
    }
}

bool RenderThreadInfoVk::onLoad(Stream* stream) {
    if(stream->getByte()) {
        m_snapshotCorrupted = true;
    }
    return false;
}

void RenderThreadInfoVk::setSnapshotCorrupted() {
    m_snapshotCorrupted = true;

    // also set the per process snapshot Info to be corrupted as well
    auto* ptr = FrameBuffer::getFB()->createProcessVkSnapshotInfo();
    if (ptr) {
        ptr->snapshotCorrupted = true;
    }
}

bool RenderThreadInfoVk::isSnapshotCorrupted() const {
    if(m_snapshotCorrupted) {
        return true;
    }

    // check per process snapshot info is corrupted
    auto* ptr = FrameBuffer::getFB()->getProcessVkSnapshotInfo();
    if (!ptr) {
        return false;
    }

    return ptr->snapshotCorrupted;
}

}  // namespace vk
}  // namespace gfxstream
