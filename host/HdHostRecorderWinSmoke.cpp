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

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "HdHostRecorder.h"

namespace {

uint32_t readBigEndian(const uint8_t* bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
}

bool validateMp4(const std::string& path, uint64_t& size, bool& hasMedia, bool& hasMovie) {
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    LARGE_INTEGER length = {};
    if (!GetFileSizeEx(file, &length) || length.QuadPart < 12) {
        CloseHandle(file);
        return false;
    }
    size = static_cast<uint64_t>(length.QuadPart);
    uint64_t offset = 0;
    bool first = true;
    while (offset + 8 <= size) {
        LARGE_INTEGER position = {};
        position.QuadPart = static_cast<LONGLONG>(offset);
        if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN)) {
            CloseHandle(file);
            return false;
        }
        std::array<uint8_t, 16> header = {};
        DWORD read = 0;
        if (!ReadFile(file, header.data(), 8, &read, nullptr) || read != 8) {
            CloseHandle(file);
            return false;
        }
        uint64_t boxSize = readBigEndian(header.data());
        uint64_t headerSize = 8;
        if (boxSize == 1) {
            if (!ReadFile(file, header.data() + 8, 8, &read, nullptr) || read != 8) {
                CloseHandle(file);
                return false;
            }
            boxSize = (static_cast<uint64_t>(readBigEndian(header.data() + 8)) << 32) |
                      readBigEndian(header.data() + 12);
            headerSize = 16;
        } else if (boxSize == 0) {
            boxSize = size - offset;
        }
        if (boxSize < headerSize || boxSize > size - offset) {
            CloseHandle(file);
            return false;
        }
        const std::string type(reinterpret_cast<const char*>(header.data() + 4), 4);
        if (first && type != "ftyp") {
            CloseHandle(file);
            return false;
        }
        first = false;
        hasMedia = hasMedia || type == "mdat";
        hasMovie = hasMovie || type == "moov";
        offset += boxSize;
    }
    CloseHandle(file);
    return offset == size && hasMedia && hasMovie;
}

}  // namespace

int main() {
    std::array<char, MAX_PATH + 1> temporary = {};
    const DWORD length = GetTempPathA(static_cast<DWORD>(temporary.size()), temporary.data());
    if (length == 0 || length >= temporary.size()) {
        std::fprintf(stderr, "resolve Windows temporary directory failed\n");
        return 1;
    }
    const std::string output = std::string(temporary.data(), length) + "hd-recorder-probe-" +
                               std::to_string(GetCurrentProcessId()) + ".mp4";
    (void)DeleteFileA(output.c_str());
    const std::string error = gfxstream::runHdHostRecorderWindowsProbe(output);
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    uint64_t size = 0;
    bool hasMedia = false;
    bool hasMovie = false;
    const bool valid = validateMp4(output, size, hasMedia, hasMovie);
    const bool removed = DeleteFileA(output.c_str()) == TRUE;
    if (!valid || !removed) {
        std::fprintf(stderr, "Windows hardware H.264 probe MP4 validation or cleanup failed\n");
        return 1;
    }
    std::printf(
        "{\"gate\":\"windows-host-recorder-smoke\",\"status\":\"pass\","
        "\"hardware_h264\":true,\"frames\":4,\"size_bytes\":%llu,"
        "\"ftyp\":true,\"mdat\":true,\"moov\":true,\"temporary_removed\":true}\n",
        static_cast<unsigned long long>(size));
    return 0;
}
