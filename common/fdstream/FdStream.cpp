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

#include "gfxstream/FdStream.h"

#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

#include <algorithm>

namespace gfxstream {

FdStream::FdStream(int fd) : mFd(fd) {}

/*static*/ FdStream FdStream::Read(const std::string& path) {
    int fd = TEMP_FAILURE_RETRY(open(path.c_str(), O_RDONLY, 0));
    return FdStream(fd);
}

/*static*/ FdStream FdStream::Write(const std::string& path) {
    int fd = TEMP_FAILURE_RETRY(open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0));
    return FdStream(fd);
}

FdStream::~FdStream() { reset(); }

void FdStream::reset() {
    if (mFd != -1) {
        close(mFd);
        mFd = -1;
    }
}

FdStream::FdStream(FdStream&& rhs) {
    reset();
    std::swap(mFd, rhs.mFd);
}

FdStream& FdStream::operator=(FdStream&& rhs) {
    reset();
    std::swap(mFd, rhs.mFd);
    return *this;
}

ssize_t FdStream::read(void* buffer, size_t byte_count) {
    uint8_t* buf = reinterpret_cast<uint8_t*>(buffer);
    size_t remaining = byte_count;
    while (remaining > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(::read(mFd, buf, remaining));
        if (n == 0) {  // EOF
            errno = ENODATA;
            return byte_count - remaining;
        }
        if (n == -1) {
            return byte_count - remaining;
        }
        buf += n;
        remaining -= n;
    }
    return byte_count - remaining;
}

ssize_t FdStream::write(const void* buffer, size_t byte_count) {
    const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer);
    size_t remaining = byte_count;
    while (remaining > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(::write(mFd, buf, remaining));
        if (n == -1) {
            return byte_count - remaining;
        }
        buf += n;
        remaining -= n;
    }
    return byte_count - remaining;
}

}  // namespace gfxstream
