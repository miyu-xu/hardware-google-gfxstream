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

#include "HdHostRecorder.h"

#ifdef _WIN32

#include <windows.h>

#include <aclapi.h>
#include <codecapi.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <sddl.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "FrameBuffer.h"
#include "HdDisplaySelection.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace {

using Microsoft::WRL::ComPtr;

constexpr uint32_t kRequestMagic = 0x48445243;   // HDRC
constexpr uint32_t kResponseMagic = 0x48445253;  // HDRS
constexpr uint16_t kProtocolVersion = 2;
constexpr uint16_t kOperationStart = 1;
constexpr uint16_t kOperationStop = 2;
constexpr uint16_t kOperationStatus = 3;
constexpr uint16_t kStatusOk = 0;
constexpr uint16_t kStatusError = 1;
constexpr uint32_t kMaxPathBytes = 4096;
constexpr uint32_t kMaxDurationSeconds = 180;
constexpr uint32_t kFramesPerSecond = 30;
constexpr DWORD kControlIoTimeoutMs = 5000;
constexpr DWORD kFinalizeTimeoutMs = 15000;
constexpr LONGLONG kOneSecond = 10'000'000;
constexpr LONGLONG kFrameDuration = kOneSecond / kFramesPerSecond;

#pragma pack(push, 1)
struct RequestHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t operation;
    uint32_t displayId;
    uint32_t maxDurationSeconds;
    uint32_t pathLength;
};

struct ResponseHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t status;
    uint64_t encodedFrames;
    uint64_t droppedFrames;
    uint32_t messageLength;
};
#pragma pack(pop)

static_assert(sizeof(RequestHeader) == 20);
static_assert(sizeof(ResponseHeader) == 28);

std::string hresultDescription(const char* operation, HRESULT result) {
    char buffer[96] = {};
    snprintf(buffer, sizeof(buffer), "%s failed with HRESULT 0x%08lx", operation,
             static_cast<unsigned long>(result));
    return buffer;
}

std::optional<std::wstring> utf8ToWide(const std::string& input) {
    if (input.empty() || input.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                                             static_cast<int>(input.size()), nullptr, 0);
    if (required <= 0) {
        return std::nullopt;
    }
    std::wstring output(static_cast<size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                            static_cast<int>(input.size()), output.data(), required) != required) {
        return std::nullopt;
    }
    return output;
}

class OwnerOnlySecurity {
   public:
    OwnerOnlySecurity() {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return;
        }
        DWORD bytes = 0;
        (void)GetTokenInformation(token, TokenUser, nullptr, 0, &bytes);
        mTokenUser.resize(bytes);
        if (bytes == 0 ||
            !GetTokenInformation(token, TokenUser, mTokenUser.data(), bytes, &bytes)) {
            CloseHandle(token);
            mTokenUser.clear();
            return;
        }
        CloseHandle(token);
        auto* tokenUser = reinterpret_cast<TOKEN_USER*>(mTokenUser.data());
        DWORD systemSidBytes = SECURITY_MAX_SID_SIZE;
        mSystemSid.resize(systemSidBytes);
        if (!CreateWellKnownSid(WinLocalSystemSid, nullptr, mSystemSid.data(), &systemSidBytes)) {
            mTokenUser.clear();
            mSystemSid.clear();
            return;
        }
        mSystemSid.resize(systemSidBytes);
        EXPLICIT_ACCESSW access[2] = {};
        for (auto& entry : access) {
            entry.grfAccessPermissions = GENERIC_ALL;
            entry.grfAccessMode = SET_ACCESS;
            entry.grfInheritance = NO_INHERITANCE;
            entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            entry.Trustee.TrusteeType = TRUSTEE_IS_USER;
        }
        access[0].Trustee.ptstrName = static_cast<LPWSTR>(tokenUser->User.Sid);
        access[1].Trustee.ptstrName = reinterpret_cast<LPWSTR>(mSystemSid.data());
        if (SetEntriesInAclW(2, access, nullptr, &mAcl) != ERROR_SUCCESS || !mAcl ||
            !InitializeSecurityDescriptor(&mDescriptor, SECURITY_DESCRIPTOR_REVISION) ||
            !SetSecurityDescriptorDacl(&mDescriptor, TRUE, mAcl, FALSE)) {
            if (mAcl) {
                LocalFree(mAcl);
                mAcl = nullptr;
            }
            mTokenUser.clear();
            mSystemSid.clear();
            return;
        }
        mAttributes = {
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = &mDescriptor,
            .bInheritHandle = FALSE,
        };
        mReady = true;
    }

    ~OwnerOnlySecurity() {
        if (mAcl) {
            LocalFree(mAcl);
        }
    }

    OwnerOnlySecurity(const OwnerOnlySecurity&) = delete;
    OwnerOnlySecurity& operator=(const OwnerOnlySecurity&) = delete;

    bool ready() const { return mReady; }
    SECURITY_ATTRIBUTES* attributes() { return mReady ? &mAttributes : nullptr; }

    bool restrictFile(const std::wstring& path) const {
        if (!mReady) {
            return false;
        }
        return SetNamedSecurityInfoW(
                   const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
                   DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr,
                   nullptr, mAcl, nullptr) == ERROR_SUCCESS;
    }

   private:
    std::vector<uint8_t> mTokenUser;
    std::vector<uint8_t> mSystemSid;
    PACL mAcl = nullptr;
    SECURITY_DESCRIPTOR mDescriptor = {};
    SECURITY_ATTRIBUTES mAttributes = {};
    bool mReady = false;
};

bool pipeIoFull(HANDLE pipe, HANDLE shutdownEvent, bool writing, void* buffer, size_t size) {
    auto* bytes = static_cast<uint8_t*>(buffer);
    size_t offset = 0;
    while (offset < size) {
        const DWORD chunk = static_cast<DWORD>(
            std::min<size_t>(size - offset, std::numeric_limits<DWORD>::max()));
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!event) {
            return false;
        }
        OVERLAPPED operation = {};
        operation.hEvent = event;
        DWORD transferred = 0;
        BOOL completed = writing ? WriteFile(pipe, bytes + offset, chunk, &transferred, &operation)
                                 : ReadFile(pipe, bytes + offset, chunk, &transferred, &operation);
        if (!completed && GetLastError() == ERROR_IO_PENDING) {
            const HANDLE waits[] = {event, shutdownEvent};
            const DWORD wait = WaitForMultipleObjects(2, waits, FALSE, kControlIoTimeoutMs);
            if (wait == WAIT_OBJECT_0) {
                completed = GetOverlappedResult(pipe, &operation, &transferred, FALSE);
            } else {
                (void)CancelIoEx(pipe, &operation);
                (void)WaitForSingleObject(event, kControlIoTimeoutMs);
                CloseHandle(event);
                return false;
            }
        }
        CloseHandle(event);
        if (!completed || transferred == 0) {
            return false;
        }
        offset += transferred;
    }
    return true;
}

bool readFull(HANDLE pipe, HANDLE shutdownEvent, void* output, size_t size) {
    return pipeIoFull(pipe, shutdownEvent, false, output, size);
}

bool writeFull(HANDLE pipe, HANDLE shutdownEvent, const void* input, size_t size) {
    return pipeIoFull(pipe, shutdownEvent, true, const_cast<void*>(input), size);
}

class SinkWriterCallback final : public IMFSinkWriterCallback {
   public:
    SinkWriterCallback() : mEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interfaceId, void** output) override {
        if (!output) {
            return E_POINTER;
        }
        if (interfaceId == __uuidof(IUnknown) || interfaceId == __uuidof(IMFSinkWriterCallback)) {
            *output = static_cast<IMFSinkWriterCallback*>(this);
            AddRef();
            return S_OK;
        }
        *output = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++mReferences; }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG references = --mReferences;
        if (references == 0) {
            delete this;
        }
        return references;
    }

    HRESULT STDMETHODCALLTYPE OnFinalize(HRESULT status) override {
        mStatus.store(status, std::memory_order_release);
        if (mEvent) {
            SetEvent(mEvent);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnMarker(DWORD, void*) override { return S_OK; }

    HRESULT wait() const {
        if (!mEvent || WaitForSingleObject(mEvent, kFinalizeTimeoutMs) != WAIT_OBJECT_0) {
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        }
        return mStatus.load(std::memory_order_acquire);
    }

   private:
    ~SinkWriterCallback() {
        if (mEvent) {
            CloseHandle(mEvent);
        }
    }

    std::atomic<ULONG> mReferences{1};
    HANDLE mEvent = nullptr;
    std::atomic<HRESULT> mStatus{E_PENDING};
};

struct PendingFrame {
    uint32_t width = 0;
    uint32_t height = 0;
    LONGLONG timestamp = 0;
    std::vector<uint8_t> pixels;
};

bool isNearBlackFrame(const std::vector<uint8_t>& pixels) {
    // Recording frames are tightly packed ARGB32. Sample the image uniformly instead of scanning
    // every pixel on gfxstream's post-callback thread. A normal Android frame always has some
    // status/navigation/UI pixels above this threshold; an all-black compositor buffer does not.
    constexpr size_t kPixelStride = 64;
    constexpr uint8_t kBlackChannelThreshold = 8;
    const size_t pixelCount = pixels.size() / 4;
    if (pixelCount == 0) return false;
    size_t sampled = 0;
    size_t nonBlack = 0;
    for (size_t pixel = 0; pixel < pixelCount; pixel += kPixelStride) {
        const size_t offset = pixel * 4;
        ++sampled;
        if (pixels[offset] > kBlackChannelThreshold ||
            pixels[offset + 1] > kBlackChannelThreshold ||
            pixels[offset + 2] > kBlackChannelThreshold) {
            ++nonBlack;
        }
    }
    // Permit a handful of noisy edge samples while still distinguishing a genuinely black
    // full-frame buffer from dark Android content.
    return nonBlack * 1000 <= sampled;
}

bool selectedEncoderIsHardware(IMFSinkWriter* writer, DWORD streamIndex) {
    ComPtr<IMFSinkWriterEx> extended;
    if (FAILED(writer->QueryInterface(IID_PPV_ARGS(&extended)))) {
        return false;
    }
    for (DWORD index = 0; index < 16; ++index) {
        GUID category = {};
        ComPtr<IMFTransform> transform;
        if (FAILED(extended->GetTransformForStream(streamIndex, index, &category, &transform))) {
            break;
        }
        if (category != MFT_CATEGORY_VIDEO_ENCODER) {
            continue;
        }
        ComPtr<IMFAttributes> attributes;
        if (FAILED(transform->GetAttributes(&attributes))) {
            return false;
        }
        UINT32 length = 0;
        if (FAILED(attributes->GetStringLength(MFT_ENUM_HARDWARE_URL_Attribute, &length)) ||
            length == 0) {
            return false;
        }
        return true;
    }
    return false;
}

struct MediaFoundationWriter {
    ComPtr<IMFSinkWriter> writer;
    ComPtr<SinkWriterCallback> callback;
    DWORD streamIndex = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    bool mediaFoundationStarted = false;
};

void releaseWriter(MediaFoundationWriter& writer) {
    writer.writer.Reset();
    writer.callback.Reset();
    if (writer.mediaFoundationStarted) {
        (void)MFShutdown();
        writer.mediaFoundationStarted = false;
    }
    writer.width = 0;
    writer.height = 0;
}

std::string createWriter(const std::wstring& path, uint32_t width, uint32_t height,
                         const OwnerOnlySecurity& security, MediaFoundationWriter& output) {
    HRESULT result = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(result)) {
        return hresultDescription("MFStartup", result);
    }
    output.mediaFoundationStarted = true;

    ComPtr<IMFAttributes> attributes;
    result = MFCreateAttributes(&attributes, 4);
    if (SUCCEEDED(result)) {
        result = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    }
    if (SUCCEEDED(result)) {
        result = attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    }
    output.callback.Attach(new SinkWriterCallback());
    if (SUCCEEDED(result)) {
        result = attributes->SetUnknown(MF_SINK_WRITER_ASYNC_CALLBACK, output.callback.Get());
    }
    if (SUCCEEDED(result)) {
        result = MFCreateSinkWriterFromURL(path.c_str(), nullptr, attributes.Get(), &output.writer);
    }
    if (FAILED(result)) {
        releaseWriter(output);
        return hresultDescription("create Media Foundation MP4 sink writer", result);
    }
    if (!security.restrictFile(path)) {
        releaseWriter(output);
        DeleteFileW(path.c_str());
        return "failed to restrict Windows screen recording to the current user";
    }

    ComPtr<IMFMediaType> encodedType;
    result = MFCreateMediaType(&encodedType);
    const uint32_t bitrate = std::clamp<uint64_t>(
        static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4, 2'000'000,
        20'000'000);
    if (SUCCEEDED(result)) result = encodedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(result)) result = encodedType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (SUCCEEDED(result)) result = encodedType->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
    if (SUCCEEDED(result)) {
        result = encodedType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(result)) result = MFSetAttributeSize(encodedType.Get(), MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(result)) {
        result = MFSetAttributeRatio(encodedType.Get(), MF_MT_FRAME_RATE, kFramesPerSecond, 1);
    }
    if (SUCCEEDED(result)) result = MFSetAttributeRatio(encodedType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(result)) result = output.writer->AddStream(encodedType.Get(), &output.streamIndex);

    ComPtr<IMFMediaType> inputType;
    if (SUCCEEDED(result)) result = MFCreateMediaType(&inputType);
    if (SUCCEEDED(result)) result = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(result)) result = inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
    if (SUCCEEDED(result)) {
        result = inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(result)) result = MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(result)) {
        result = MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, kFramesPerSecond, 1);
    }
    if (SUCCEEDED(result)) result = MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(result)) {
        result = output.writer->SetInputMediaType(output.streamIndex, inputType.Get(), nullptr);
    }
    if (SUCCEEDED(result)) result = output.writer->BeginWriting();
    if (FAILED(result)) {
        releaseWriter(output);
        DeleteFileW(path.c_str());
        return hresultDescription("configure Media Foundation H.264 writer", result);
    }
    if (!selectedEncoderIsHardware(output.writer.Get(), output.streamIndex)) {
        releaseWriter(output);
        DeleteFileW(path.c_str());
        return "Media Foundation selected a non-hardware H.264 encoder";
    }
    output.width = width;
    output.height = height;
    return {};
}

std::string writeFrame(MediaFoundationWriter& writer, const std::vector<uint8_t>& pixels,
                       LONGLONG timestamp) {
    const uint64_t expected = static_cast<uint64_t>(writer.width) * writer.height * 4;
    if (pixels.size() != expected || expected > std::numeric_limits<DWORD>::max()) {
        return "Windows screen recording frame size is invalid";
    }
    ComPtr<IMFMediaBuffer> buffer;
    HRESULT result = MFCreateMemoryBuffer(static_cast<DWORD>(expected), &buffer);
    BYTE* destination = nullptr;
    DWORD capacity = 0;
    if (SUCCEEDED(result)) result = buffer->Lock(&destination, &capacity, nullptr);
    if (SUCCEEDED(result) && capacity < expected) result = E_UNEXPECTED;
    if (SUCCEEDED(result)) {
        memcpy(destination, pixels.data(), static_cast<size_t>(expected));
        result = buffer->Unlock();
        destination = nullptr;
    } else if (destination) {
        (void)buffer->Unlock();
    }
    if (SUCCEEDED(result)) result = buffer->SetCurrentLength(static_cast<DWORD>(expected));
    ComPtr<IMFSample> sample;
    if (SUCCEEDED(result)) result = MFCreateSample(&sample);
    if (SUCCEEDED(result)) result = sample->AddBuffer(buffer.Get());
    if (SUCCEEDED(result)) result = sample->SetSampleTime(timestamp);
    if (SUCCEEDED(result)) result = sample->SetSampleDuration(kFrameDuration);
    if (SUCCEEDED(result)) result = writer.writer->WriteSample(writer.streamIndex, sample.Get());
    return FAILED(result) ? hresultDescription("write Media Foundation H.264 sample", result)
                          : std::string();
}

std::string finalizeWriter(MediaFoundationWriter& writer, const std::wstring& path,
                           const OwnerOnlySecurity& security) {
    HRESULT result = writer.writer->Finalize();
    if (SUCCEEDED(result)) {
        result = writer.callback->wait();
    }
    releaseWriter(writer);
    if (FAILED(result)) {
        DeleteFileW(path.c_str());
        return hresultDescription("finalize Media Foundation MP4", result);
    }
    if (!security.restrictFile(path)) {
        DeleteFileW(path.c_str());
        return "failed to restrict finalized Windows screen recording to the current user";
    }
    return {};
}

class HostRecorder {
   public:
    static HostRecorder& get() {
        static HostRecorder recorder;
        return recorder;
    }

    void startControl() {
        std::lock_guard<std::mutex> lock(mStateMutex);
        if (mControlThread.joinable()) {
            return;
        }
        const char* endpoint = getenv("HD_HOST_RECORDER_ENDPOINT");
        if (!endpoint || endpoint[0] == '\0') {
            return;
        }
        auto endpointWide = utf8ToWide(endpoint);
        if (!endpointWide || endpointWide->rfind(L"\\\\.\\pipe\\", 0) != 0 ||
            !mSecurity.ready()) {
            ERR("HD Windows host recorder endpoint or owner-only security is invalid");
            return;
        }
        mEndpoint = std::move(*endpointWide);
        mShutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!mShutdownEvent) {
            mEndpoint.clear();
            return;
        }
        mTimerShutdown = false;
        mTimerThread = std::thread([this] { timerLoop(); });
        mControlThread = std::thread([this] { controlLoop(); });
        INFO("HD Windows host recorder control ready");
    }

    void stopControl() {
        if (mShutdownEvent) {
            SetEvent(mShutdownEvent);
        }
        const HANDLE pipe = mControlPipe.load(std::memory_order_acquire);
        if (pipe != INVALID_HANDLE_VALUE) {
            (void)CancelIoEx(pipe, nullptr);
        }
        if (mControlThread.joinable()) {
            mControlThread.join();
        }
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mTimerShutdown = true;
            mStopRequested = false;
            mTimerDeadline.reset();
            mTimerCondition.notify_all();
        }
        if (mTimerThread.joinable()) {
            mTimerThread.join();
        }
        (void)stopRecording();
        if (mShutdownEvent) {
            CloseHandle(mShutdownEvent);
            mShutdownEvent = nullptr;
        }
        mEndpoint.clear();
    }

   private:
    HostRecorder() = default;
    ~HostRecorder() { stopControl(); }
    HostRecorder(const HostRecorder&) = delete;
    HostRecorder& operator=(const HostRecorder&) = delete;

    void timerLoop() {
        std::unique_lock<std::mutex> lock(mStateMutex);
        while (!mTimerShutdown) {
            if (mStopRequested) {
                mStopRequested = false;
                lock.unlock();
                const std::string error = stopRecording();
                if (!error.empty()) {
                    ERR("HD Windows host recorder asynchronous stop failed: %s", error.c_str());
                }
                lock.lock();
                continue;
            }
            if (!mTimerDeadline) {
                mTimerCondition.wait(lock, [this] {
                    return mTimerShutdown || mStopRequested || mTimerDeadline.has_value();
                });
                continue;
            }
            const auto deadline = *mTimerDeadline;
            const bool interrupted = mTimerCondition.wait_until(lock, deadline, [this, deadline] {
                return mTimerShutdown || mStopRequested || !mTimerDeadline ||
                       *mTimerDeadline != deadline;
            });
            if (interrupted) {
                continue;
            }
            mTimerDeadline.reset();
            lock.unlock();
            const std::string error = stopRecording();
            if (!error.empty()) {
                ERR("HD Windows host recorder automatic stop failed: %s", error.c_str());
            }
            lock.lock();
        }
    }

    HANDLE createControlPipe() {
        return CreateNamedPipeW(
            mEndpoint.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 8192,
            8192, 0, mSecurity.attributes());
    }

    bool connectClient(HANDLE pipe) const {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!event) {
            return false;
        }
        OVERLAPPED operation = {};
        operation.hEvent = event;
        BOOL connected = ConnectNamedPipe(pipe, &operation);
        if (!connected && GetLastError() == ERROR_PIPE_CONNECTED) {
            connected = TRUE;
        } else if (!connected && GetLastError() == ERROR_IO_PENDING) {
            const HANDLE waits[] = {event, mShutdownEvent};
            const DWORD wait = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0) {
                DWORD ignored = 0;
                connected = GetOverlappedResult(pipe, &operation, &ignored, FALSE);
            } else {
                (void)CancelIoEx(pipe, &operation);
            }
        }
        CloseHandle(event);
        return connected == TRUE;
    }

    void controlLoop() {
        while (WaitForSingleObject(mShutdownEvent, 0) != WAIT_OBJECT_0) {
            const HANDLE pipe = createControlPipe();
            if (pipe == INVALID_HANDLE_VALUE) {
                ERR("HD Windows host recorder named-pipe creation failed: %lu", GetLastError());
                return;
            }
            mControlPipe.store(pipe, std::memory_order_release);
            if (connectClient(pipe)) {
                handleClient(pipe);
                // A successful overlapped WriteFile only transfers the response into the pipe
                // buffer. DisconnectNamedPipe may discard bytes that the client has not consumed
                // yet, which made non-empty error responses appear as ERROR_NO_DATA after the
                // fixed-size header. The endpoint is owner-only and responses are bounded, so
                // flush the complete response before disconnecting this one-request session.
                (void)FlushFileBuffers(pipe);
                (void)DisconnectNamedPipe(pipe);
            }
            mControlPipe.store(INVALID_HANDLE_VALUE, std::memory_order_release);
            CloseHandle(pipe);
        }
    }

    void handleClient(HANDLE pipe) {
        RequestHeader request = {};
        if (!readFull(pipe, mShutdownEvent, &request, sizeof(request)) ||
            request.magic != kRequestMagic || request.version != kProtocolVersion ||
            request.pathLength > kMaxPathBytes) {
            sendResponse(pipe, kStatusError, "invalid HD host recorder request");
            return;
        }
        std::string path(request.pathLength, '\0');
        if (!path.empty() && !readFull(pipe, mShutdownEvent, path.data(), path.size())) {
            sendResponse(pipe, kStatusError, "truncated HD host recorder request");
            return;
        }
        std::string error;
        INFO("HD Windows host recorder received operation %u for display %u",
             static_cast<unsigned>(request.operation), request.displayId);
        switch (request.operation) {
            case kOperationStart:
                if (path.empty() || request.maxDurationSeconds == 0 ||
                    request.maxDurationSeconds > kMaxDurationSeconds || request.displayId >= 16) {
                    error = "invalid host recording display, path, or duration";
                } else {
                    error = startRecording(path, request.displayId, request.maxDurationSeconds);
                }
                break;
            case kOperationStop:
                error = path.empty() ? stopRecording() : "stop request must not include a path";
                break;
            case kOperationStatus:
                if (!path.empty()) {
                    error = "status request must not include a path";
                }
                break;
            default:
                error = "unknown host recorder operation";
                break;
        }
        sendResponse(pipe, error.empty() ? kStatusOk : kStatusError, error);
    }

    void sendResponse(HANDLE pipe, uint16_t status, const std::string& message) {
        uint64_t encodedFrames = 0;
        uint64_t droppedFrames = 0;
        bool initialStaticFrame = false;
        int initialFrameYDirection = 0;
        uint64_t nearBlackFrames = 0;
        uint64_t maxConsecutiveNearBlackFrames = 0;
        uint64_t maxSourceFrameGapMillis = 0;
        uint64_t sourceFrameGapsOver100Millis = 0;
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            encodedFrames = mEncodedFrames;
            droppedFrames = mDroppedFrames;
            initialStaticFrame = mInitialStaticFrame;
            initialFrameYDirection = mInitialFrameYDirection;
            nearBlackFrames = mNearBlackFrames;
            maxConsecutiveNearBlackFrames = mMaxConsecutiveNearBlackFrames;
            maxSourceFrameGapMillis = mMaxSourceFrameGapMillis;
            sourceFrameGapsOver100Millis = mSourceFrameGapsOver100Millis;
        }
        std::string responseMessage = message;
        if (status == kStatusOk) {
            char evidence[512] = {};
            snprintf(evidence, sizeof(evidence),
                     "{\"initial_static_frame\":%s,\"initial_frame_y_direction\":%d,"
                     "\"near_black_frames\":%llu,"
                     "\"max_consecutive_near_black_frames\":%llu,"
                     "\"max_source_frame_gap_millis\":%llu,"
                     "\"source_frame_gaps_over_100_millis\":%llu}",
                     initialStaticFrame ? "true" : "false", initialFrameYDirection,
                     static_cast<unsigned long long>(nearBlackFrames),
                     static_cast<unsigned long long>(maxConsecutiveNearBlackFrames),
                     static_cast<unsigned long long>(maxSourceFrameGapMillis),
                     static_cast<unsigned long long>(sourceFrameGapsOver100Millis));
            responseMessage = evidence;
        }
        const size_t boundedLength = std::min<size_t>(responseMessage.size(), kMaxPathBytes);
        const ResponseHeader response = {
            .magic = kResponseMagic,
            .version = kProtocolVersion,
            .status = status,
            .encodedFrames = encodedFrames,
            .droppedFrames = droppedFrames,
            .messageLength = static_cast<uint32_t>(boundedLength),
        };
        std::vector<uint8_t> wire(sizeof(response) + boundedLength);
        memcpy(wire.data(), &response, sizeof(response));
        if (boundedLength != 0) {
            memcpy(wire.data() + sizeof(response), responseMessage.data(), boundedLength);
        }
        (void)writeFull(pipe, mShutdownEvent, wire.data(), wire.size());
    }

    std::string startRecording(const std::string& path, uint32_t displayId,
                               uint32_t maxDurationSeconds) {
        std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
        auto outputPath = utf8ToWide(path);
        if (!outputPath) {
            return "Windows host recording path is not valid UTF-8";
        }
        const uint32_t callbackDisplayId = hdPhysicalDisplayIdForScanout(displayId);
        INFO("HD Windows host recording registering scanout %u as display %u", displayId,
             callbackDisplayId);
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if (mAcceptFrames || mCallbackRegistered || mEncoderThread.joinable() ||
                !mOutputPath.empty()) {
                return "one HD host recording is already active";
            }
            mOutputPath = std::move(*outputPath);
            mDisplayId = callbackDisplayId;
            mEncodedFrames = 0;
            mDroppedFrames = 0;
            mInitialStaticFrame = false;
            mInitialFrameYDirection = 0;
            mNearBlackFrames = 0;
            mConsecutiveNearBlackFrames = 0;
            mMaxConsecutiveNearBlackFrames = 0;
            mMaxSourceFrameGapMillis = 0;
            mSourceFrameGapsOver100Millis = 0;
            mHasCapturedFrame = false;
            mHasFinalizedRecording = false;
            mLastCompletionError.clear();
            mFrameFailure.clear();
            mPendingFrame.reset();
            mEncoderStop = false;
            mStartTime = std::chrono::steady_clock::now();
            mLastFrameTime = mStartTime - std::chrono::seconds(1);
            mAcceptFrames = true;
            mCallbackRegistered = true;
            ++mGeneration;
            mEncoderThread = std::thread([this] { encoderLoop(); });
        }
        FrameBuffer* fb = FrameBuffer::getFB();
        INFO("HD Windows host recording entering FrameBuffer post-callback registration");
        if (!fb || !fb->setPostCallback(&HostRecorder::onFrame, this, callbackDisplayId, true,
                                        kFramesPerSecond)) {
            {
                std::lock_guard<std::mutex> lock(mStateMutex);
                mAcceptFrames = false;
                mCallbackRegistered = false;
                mEncoderStop = true;
                mEncoderCondition.notify_all();
            }
            if (mEncoderThread.joinable()) {
                mEncoderThread.join();
            }
            std::lock_guard<std::mutex> lock(mStateMutex);
            mOutputPath.clear();
            mDisplayId = 0;
            return fb ? "gfxstream display is already configured for recording"
                      : "gfxstream FrameBuffer is unavailable";
        }
        INFO("HD Windows host recording completed FrameBuffer post-callback registration");
        const bool initialStaticFrame = fb->capturePostCallbackFrame(callbackDisplayId);
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mInitialStaticFrame = initialStaticFrame;
        }
        if (initialStaticFrame) {
            INFO("HD Windows host recording captured the current static display frame");
        } else {
            INFO("HD Windows host recording is waiting for the display's first posted frame");
        }
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mTimerDeadline = std::chrono::steady_clock::now() +
                             std::chrono::seconds(maxDurationSeconds);
            mTimerCondition.notify_all();
        }
        INFO("HD Windows host recording started for scanout %u", displayId);
        return {};
    }

    std::string stopRecording() {
        std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
        bool shouldUnregister = false;
        uint32_t displayId = 0;
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            shouldUnregister = mCallbackRegistered;
            displayId = mDisplayId;
            mAcceptFrames = false;
            mCallbackRegistered = false;
            mStopRequested = false;
            mTimerDeadline.reset();
            ++mGeneration;
            mTimerCondition.notify_all();
        }
        if (shouldUnregister) {
            if (FrameBuffer* fb = FrameBuffer::getFB()) {
                (void)fb->setPostCallback(nullptr, nullptr, displayId, true);
                if (!fb->repost()) {
                    ERR("HD Windows host recorder could not repost display %u after recording",
                        displayId);
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mEncoderStop = true;
            mEncoderCondition.notify_all();
        }
        if (mEncoderThread.joinable()) {
            mEncoderThread.join();
        }
        std::lock_guard<std::mutex> lock(mStateMutex);
        if (mOutputPath.empty()) {
            return mHasFinalizedRecording ? mLastCompletionError
                                          : "no HD host recording is active";
        }
        mOutputPath.clear();
        mDisplayId = 0;
        mPendingFrame.reset();
        return mLastCompletionError;
    }

    static void onFrame(void* context, uint32_t, int width, int height, int ydir, int, int,
                        unsigned char* pixels) {
        static_cast<HostRecorder*>(context)->appendFrame(width, height, ydir, pixels);
    }

    void appendFrame(int width, int height, int ydir, const unsigned char* pixels) {
        if (!pixels || width < 2 || height < 2) {
            return;
        }
        std::lock_guard<std::mutex> lock(mStateMutex);
        if (!mAcceptFrames) {
            return;
        }
        if (mInitialFrameYDirection == 0) {
            mInitialFrameYDirection = ydir;
        }
        const auto now = std::chrono::steady_clock::now();
        if (now - mLastFrameTime < std::chrono::milliseconds(1000 / kFramesPerSecond)) {
            return;
        }
        if (mHasCapturedFrame) {
            const uint64_t gapMillis = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastFrameTime)
                    .count());
            mMaxSourceFrameGapMillis = std::max(mMaxSourceFrameGapMillis, gapMillis);
            if (gapMillis > 100) {
                ++mSourceFrameGapsOver100Millis;
            }
        } else {
            mHasCapturedFrame = true;
        }
        mLastFrameTime = now;
        const uint32_t encodedWidth = static_cast<uint32_t>(width & ~1);
        const uint32_t encodedHeight = static_cast<uint32_t>(height & ~1);
        const uint64_t byteCount = static_cast<uint64_t>(encodedWidth) * encodedHeight * 4;
        if (byteCount > std::numeric_limits<size_t>::max()) {
            ++mDroppedFrames;
            return;
        }
        PendingFrame frame = {
            .width = encodedWidth,
            .height = encodedHeight,
            .timestamp = std::chrono::duration_cast<std::chrono::duration<LONGLONG,
                                                                            std::ratio<1, kOneSecond>>>(
                             now - mStartTime)
                             .count(),
            .pixels = std::vector<uint8_t>(static_cast<size_t>(byteCount)),
        };
        const size_t sourceStride = static_cast<size_t>(width) * 4;
        const size_t destinationStride = static_cast<size_t>(encodedWidth) * 4;
        for (uint32_t row = 0; row < encodedHeight; ++row) {
            const uint32_t sourceRow = ydir < 0 ? encodedHeight - 1 - row : row;
            memcpy(frame.pixels.data() + static_cast<size_t>(row) * destinationStride,
                   pixels + static_cast<size_t>(sourceRow) * sourceStride, destinationStride);
        }
        if (isNearBlackFrame(frame.pixels)) {
            ++mNearBlackFrames;
            ++mConsecutiveNearBlackFrames;
            mMaxConsecutiveNearBlackFrames =
                std::max(mMaxConsecutiveNearBlackFrames, mConsecutiveNearBlackFrames);
        } else {
            mConsecutiveNearBlackFrames = 0;
        }
        if (mPendingFrame) {
            ++mDroppedFrames;
        }
        mPendingFrame = std::move(frame);
        mEncoderCondition.notify_one();
    }

    void encoderLoop() {
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        MediaFoundationWriter writer;
        std::vector<uint8_t> lastPixels;
        LONGLONG lastTimestamp = -1;
        std::string error;
        while (error.empty()) {
            std::optional<PendingFrame> frame;
            {
                std::unique_lock<std::mutex> lock(mStateMutex);
                mEncoderCondition.wait(lock,
                                       [this] { return mEncoderStop || mPendingFrame.has_value(); });
                if (mPendingFrame) {
                    frame = std::move(mPendingFrame);
                    mPendingFrame.reset();
                } else if (mEncoderStop) {
                    break;
                }
            }
            if (!frame) {
                continue;
            }
            if (FAILED(comResult)) {
                error = hresultDescription("initialize Windows recorder COM apartment", comResult);
            } else if (!writer.writer) {
                error = createWriter(mOutputPath, frame->width, frame->height, mSecurity, writer);
            }
            if (error.empty() &&
                (frame->width != writer.width || frame->height != writer.height)) {
                error = "Windows screen recording dimensions changed during capture";
            }
            if (error.empty()) {
                frame->timestamp = std::max(frame->timestamp, lastTimestamp + 1);
                error = writeFrame(writer, frame->pixels, frame->timestamp);
            }
            {
                std::lock_guard<std::mutex> lock(mStateMutex);
                if (error.empty()) {
                    lastTimestamp = frame->timestamp;
                    lastPixels = std::move(frame->pixels);
                    ++mEncodedFrames;
                } else {
                    ++mDroppedFrames;
                    mFrameFailure = error;
                    mAcceptFrames = false;
                    mStopRequested = true;
                    mTimerCondition.notify_all();
                }
            }
        }

        if (error.empty() && writer.writer) {
            const LONGLONG terminal = std::chrono::duration_cast<
                std::chrono::duration<LONGLONG, std::ratio<1, kOneSecond>>>(
                                          std::chrono::steady_clock::now() - mStartTime)
                                          .count();
            const LONGLONG anchor = terminal - kFrameDuration;
            if (!lastPixels.empty() && anchor > lastTimestamp) {
                error = writeFrame(writer, lastPixels, anchor);
                if (error.empty()) {
                    std::lock_guard<std::mutex> lock(mStateMutex);
                    ++mEncodedFrames;
                    lastTimestamp = anchor;
                }
            }
            if (error.empty() && !lastPixels.empty() && terminal > lastTimestamp) {
                error = writeFrame(writer, lastPixels, terminal);
                if (error.empty()) {
                    std::lock_guard<std::mutex> lock(mStateMutex);
                    ++mEncodedFrames;
                }
            }
        } else if (error.empty()) {
            error = mFrameFailure.empty() ? "HD host recording did not receive a frame"
                                          : mFrameFailure;
        }
        if (writer.writer) {
            const std::string finalizeError = finalizeWriter(writer, mOutputPath, mSecurity);
            if (error.empty()) {
                error = finalizeError;
            }
        }
        if (FAILED(comResult)) {
            // The COM apartment was not initialized, so there is nothing to uninitialize.
        } else {
            CoUninitialize();
        }
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mLastCompletionError = std::move(error);
            mHasFinalizedRecording = true;
            if (mLastCompletionError.empty()) {
                INFO("HD Windows host recording finalized (%llu frames, %llu dropped)",
                     static_cast<unsigned long long>(mEncodedFrames),
                     static_cast<unsigned long long>(mDroppedFrames));
            }
        }
    }

    // Serializes control-client STOP, automatic timeout, encoder-failure cleanup, and teardown.
    // In particular, two callers must never join the same encoder thread concurrently.
    std::mutex mLifecycleMutex;
    std::mutex mStateMutex;
    OwnerOnlySecurity mSecurity;
    std::wstring mEndpoint;
    HANDLE mShutdownEvent = nullptr;
    std::atomic<HANDLE> mControlPipe{INVALID_HANDLE_VALUE};
    std::thread mControlThread;
    std::thread mTimerThread;
    std::thread mEncoderThread;
    std::condition_variable mTimerCondition;
    std::condition_variable mEncoderCondition;
    std::optional<std::chrono::steady_clock::time_point> mTimerDeadline;
    std::optional<PendingFrame> mPendingFrame;
    bool mTimerShutdown = false;
    bool mStopRequested = false;
    bool mEncoderStop = false;
    bool mAcceptFrames = false;
    bool mCallbackRegistered = false;
    bool mHasFinalizedRecording = false;
    std::wstring mOutputPath;
    uint32_t mDisplayId = 0;
    std::atomic<uint64_t> mGeneration{0};
    std::string mLastCompletionError;
    std::string mFrameFailure;
    uint64_t mEncodedFrames = 0;
    uint64_t mDroppedFrames = 0;
    bool mInitialStaticFrame = false;
    int mInitialFrameYDirection = 0;
    uint64_t mNearBlackFrames = 0;
    uint64_t mConsecutiveNearBlackFrames = 0;
    uint64_t mMaxConsecutiveNearBlackFrames = 0;
    uint64_t mMaxSourceFrameGapMillis = 0;
    uint64_t mSourceFrameGapsOver100Millis = 0;
    bool mHasCapturedFrame = false;
    std::chrono::steady_clock::time_point mStartTime;
    std::chrono::steady_clock::time_point mLastFrameTime;
};

}  // namespace

std::string runHdHostRecorderWindowsProbe(const std::string& outputPath) {
    const auto path = utf8ToWide(outputPath);
    if (!path) {
        return "Windows recorder probe path is not valid UTF-8";
    }
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult)) {
        return hresultDescription("initialize Windows recorder probe COM apartment", comResult);
    }
    OwnerOnlySecurity security;
    MediaFoundationWriter writer;
    std::string error;
    if (!security.ready()) {
        error = "Windows recorder probe could not create owner-only security";
    } else {
        error = createWriter(*path, 640, 480, security, writer);
    }
    std::vector<uint8_t> pixels(640 * 480 * 4);
    for (uint32_t frame = 0; error.empty() && frame < 4; ++frame) {
        for (uint32_t y = 0; y < 480; ++y) {
            for (uint32_t x = 0; x < 640; ++x) {
                const size_t offset = (static_cast<size_t>(y) * 640 + x) * 4;
                pixels[offset] = static_cast<uint8_t>((x + frame * 31) & 0xff);
                pixels[offset + 1] = static_cast<uint8_t>((y + frame * 47) & 0xff);
                pixels[offset + 2] = static_cast<uint8_t>((x + y + frame * 59) & 0xff);
                pixels[offset + 3] = 0xff;
            }
        }
        error = writeFrame(writer, pixels, static_cast<LONGLONG>(frame) * kOneSecond / 2);
    }
    if (writer.writer) {
        const std::string finalizeError = finalizeWriter(writer, *path, security);
        if (error.empty()) {
            error = finalizeError;
        }
    }
    if (!error.empty()) {
        DeleteFileW(path->c_str());
    }
    CoUninitialize();
    return error;
}

void startHdHostRecorderControl() { HostRecorder::get().startControl(); }

void stopHdHostRecorderControl() { HostRecorder::get().stopControl(); }

}  // namespace gfxstream

#endif
