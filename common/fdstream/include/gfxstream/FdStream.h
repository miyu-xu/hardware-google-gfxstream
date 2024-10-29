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

#pragma once

#include <string>

#include "aemu/base/files/Stream.h"

namespace gfxstream {

class FdStream : public android::base::Stream {
   public:
    static FdStream Read(const std::string& path);
    static FdStream Write(const std::string& path);

    virtual ~FdStream();

    FdStream(const FdStream& rhs) = delete;
    FdStream& operator=(const FdStream& rhs) = delete;

    FdStream(FdStream&& rhs);
    FdStream& operator=(FdStream&& rhs);

    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;

   private:
    FdStream(int fd);

    void reset();

    int mFd = -1;
};

}  // namespace gfxstream