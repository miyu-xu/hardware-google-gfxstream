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

#include "HdFrameBridgeWin32.h"

#ifdef _WIN32

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>

#include "host-common/logging.h"
#include "vulkan/VkCommonOperations.h"

namespace gfxstream {
namespace {

constexpr uint32_t kWireMagic = 0x32464448;  // HDF2 in little endian.
constexpr uint16_t kWireVersion = 2;
constexpr uint16_t kRegister = 1;
constexpr uint16_t kPublish = 2;
constexpr uint16_t kRelease = 3;
constexpr uint8_t kBufferCount = 3;
constexpr DWORD kIoTimeoutMs = 5000;

#pragma pack(push, 1)
struct WireHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t kind;
    uint32_t size;
};

struct RegisterMessage {
    WireHeader header;
    uint8_t bufferId;
    uint8_t dedicatedAllocation;
    uint8_t linearTiling;
    uint8_t luidValid;
    uint32_t memoryTypeIndex;
    uint32_t width;
    uint32_t height;
    uint32_t strideBytes;
    uint32_t virglFormat;
    uint32_t imageCreateFlags;
    uint32_t imageUsage;
    uint32_t imageFormat;
    uint64_t allocationSize;
    uint64_t memoryHandle;
    std::array<uint8_t, VK_LUID_SIZE> deviceLuid;
};

struct PublishMessage {
    WireHeader header;
    uint8_t bufferId;
    uint8_t reserved[7];
    uint64_t acquireValue;
};

struct ReleaseMessage {
    WireHeader header;
    uint8_t bufferId;
    uint8_t accepted;
    uint8_t reserved[6];
    uint64_t releaseValue;
};
#pragma pack(pop)

static_assert(sizeof(WireHeader) == 12);
static_assert(sizeof(RegisterMessage) == 72);
static_assert(sizeof(PublishMessage) == 28);
static_assert(sizeof(ReleaseMessage) == 28);

bool writeAll(HANDLE pipe, const void* data, size_t size) {
    const auto* cursor = static_cast<const uint8_t*>(data);
    while (size != 0) {
        const DWORD chunk =
            static_cast<DWORD>(std::min<size_t>(size, std::numeric_limits<DWORD>::max()));
        DWORD written = 0;
        if (!WriteFile(pipe, cursor, chunk, &written, nullptr) || written == 0) {
            return false;
        }
        cursor += written;
        size -= written;
    }
    return true;
}

bool readAll(HANDLE pipe, void* data, size_t size) {
    auto* cursor = static_cast<uint8_t*>(data);
    while (size != 0) {
        const DWORD chunk =
            static_cast<DWORD>(std::min<size_t>(size, std::numeric_limits<DWORD>::max()));
        DWORD read = 0;
        if (!ReadFile(pipe, cursor, chunk, &read, nullptr) || read == 0) {
            return false;
        }
        cursor += read;
        size -= read;
    }
    return true;
}

class HdFrameBridgeWin32Impl final : public HdFrameBridgeWin32 {
   public:
    explicit HdFrameBridgeWin32Impl(std::string endpoint) : mEndpoint(std::move(endpoint)) {}

    ~HdFrameBridgeWin32Impl() override {
        if (mPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(mPipe);
        }
        if (mBrokerProcess) {
            CloseHandle(mBrokerProcess);
        }
    }

    bool publish(uint32_t resourceHandle, uint32_t width, uint32_t height,
                 uint32_t virglFormat) override {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!connectLocked()) {
            return false;
        }

        auto slot = mSlots.find(resourceHandle);
        if (slot == mSlots.end()) {
            if (mSlots.size() >= kBufferCount) {
                // Android normally posts three buffers. An additional resource is not silently
                // substituted because its memory was not negotiated with the strict broker.
                return false;
            }
            const uint8_t bufferId = static_cast<uint8_t>(mSlots.size());
            if (!registerLocked(resourceHandle, bufferId, width, height, virglFormat)) {
                disconnectLocked();
                return false;
            }
            slot = mSlots.emplace(resourceHandle, bufferId).first;
        }

        const uint64_t acquireValue = ++mAcquireValue;
        const PublishMessage publish = {
            .header = {kWireMagic, kWireVersion, kPublish, sizeof(PublishMessage)},
            .bufferId = slot->second,
            .reserved = {},
            .acquireValue = acquireValue,
        };
        ReleaseMessage release = {};
        if (!writeAll(mPipe, &publish, sizeof(publish)) ||
            !readAll(mPipe, &release, sizeof(release)) || release.header.magic != kWireMagic ||
            release.header.version != kWireVersion || release.header.kind != kRelease ||
            release.header.size != sizeof(ReleaseMessage) || release.bufferId != slot->second ||
            release.accepted == 0 || release.releaseValue != acquireValue) {
            disconnectLocked();
            return false;
        }
        return true;
    }

   private:
    bool connectLocked() {
        if (mPipe != INVALID_HANDLE_VALUE) {
            return true;
        }
        if (!WaitNamedPipeA(mEndpoint.c_str(), kIoTimeoutMs)) {
            return false;
        }
        mPipe = CreateFileA(mEndpoint.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (mPipe == INVALID_HANDLE_VALUE) {
            return false;
        }
        ULONG brokerPid = 0;
        if (!GetNamedPipeServerProcessId(mPipe, &brokerPid)) {
            disconnectLocked();
            return false;
        }
        mBrokerProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, brokerPid);
        if (!mBrokerProcess) {
            disconnectLocked();
            return false;
        }
        return true;
    }

    bool registerLocked(uint32_t resourceHandle, uint8_t bufferId, uint32_t width, uint32_t height,
                        uint32_t virglFormat) {
        auto exported = vk::exportColorBufferMemory(resourceHandle);
        if (!exported) {
            ERR("HD frame broker could not export gfxstream color buffer %u", resourceHandle);
            return false;
        }
        auto rawMemory = exported->descriptor.get();
        if (!rawMemory) {
            return false;
        }
        HANDLE brokerMemory = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), *rawMemory, mBrokerProcess, &brokerMemory, 0,
                             FALSE, DUPLICATE_SAME_ACCESS)) {
            return false;
        }

        std::array<uint8_t, VK_LUID_SIZE> luid = {};
        bool luidValid = false;
        vk::getVkPhysicalDeviceLuid(luid.data(), luid.size(), &luidValid);
        RegisterMessage registration = {
            .header = {kWireMagic, kWireVersion, kRegister, sizeof(RegisterMessage)},
            .bufferId = bufferId,
            .dedicatedAllocation = static_cast<uint8_t>(exported->dedicatedAllocation),
            .linearTiling = static_cast<uint8_t>(exported->linearTiling),
            .luidValid = static_cast<uint8_t>(luidValid),
            .memoryTypeIndex = exported->memoryTypeIndex,
            .width = width,
            .height = height,
            .strideBytes = width * 4,
            .virglFormat = virglFormat,
            .imageCreateFlags = exported->imageCreateFlags,
            .imageUsage = exported->imageUsage,
            .imageFormat = static_cast<uint32_t>(exported->imageFormat),
            .allocationSize = exported->size,
            .memoryHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(brokerMemory)),
            .deviceLuid = luid,
        };
        ReleaseMessage acknowledgement = {};
        if (!writeAll(mPipe, &registration, sizeof(registration)) ||
            !readAll(mPipe, &acknowledgement, sizeof(acknowledgement)) ||
            acknowledgement.header.magic != kWireMagic ||
            acknowledgement.header.version != kWireVersion ||
            acknowledgement.header.kind != kRelease ||
            acknowledgement.header.size != sizeof(ReleaseMessage) ||
            acknowledgement.bufferId != bufferId || acknowledgement.accepted == 0 ||
            acknowledgement.releaseValue != 0) {
            // The duplicated handle belongs to the broker process; ask the broker to close it by
            // closing the connection and failing the strict frame negotiation.
            return false;
        }
        return true;
    }

    void disconnectLocked() {
        if (mPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(mPipe);
            mPipe = INVALID_HANDLE_VALUE;
        }
        if (mBrokerProcess) {
            CloseHandle(mBrokerProcess);
            mBrokerProcess = nullptr;
        }
        mSlots.clear();
        mAcquireValue = 0;
    }

    std::string mEndpoint;
    HANDLE mPipe = INVALID_HANDLE_VALUE;
    HANDLE mBrokerProcess = nullptr;
    std::mutex mMutex;
    std::unordered_map<uint32_t, uint8_t> mSlots;
    uint64_t mAcquireValue = 0;
};

}  // namespace

std::unique_ptr<HdFrameBridgeWin32> HdFrameBridgeWin32::createFromEnvironment() {
    const char* endpoint = std::getenv("HD_FRAME_BROKER_V2");
    if (!endpoint || endpoint[0] == '\0') {
        return nullptr;
    }
    return std::make_unique<HdFrameBridgeWin32Impl>(endpoint);
}

}  // namespace gfxstream

#else

namespace gfxstream {
std::unique_ptr<HdFrameBridgeWin32> HdFrameBridgeWin32::createFromEnvironment() { return nullptr; }
}  // namespace gfxstream

#endif
