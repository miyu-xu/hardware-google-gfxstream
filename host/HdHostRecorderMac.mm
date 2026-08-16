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

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <VideoToolbox/VTCompressionProperties.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "FrameBuffer.h"
#include "HdDisplaySelection.h"
#include "host-common/logging.h"

namespace gfxstream {
namespace {

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
constexpr int32_t kFramesPerSecond = 30;

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

bool readFull(int fd, void* output, size_t size) {
    auto* bytes = static_cast<uint8_t*>(output);
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = read(fd, bytes + offset, size - offset);
        if (count > 0) {
            offset += static_cast<size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}

bool writeFull(int fd, const void* input, size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(input);
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = write(fd, bytes + offset, size - offset);
        if (count > 0) {
            offset += static_cast<size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}

std::string errorDescription(NSError* error) {
    if (error == nil) {
        return "unknown AVFoundation error";
    }
    const char* utf8 = error.localizedDescription.UTF8String;
    return utf8 ? std::string(utf8) : std::string("unprintable AVFoundation error");
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
        sockaddr_un address = {};
        if (strlen(endpoint) >= sizeof(address.sun_path)) {
            ERR("HD host recorder endpoint is too long");
            return;
        }

        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            ERR("HD host recorder socket creation failed: %s", strerror(errno));
            return;
        }
        int flags = fcntl(fd, F_GETFD);
        if (flags >= 0) {
            (void)fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        }
        address.sun_family = AF_UNIX;
        strlcpy(address.sun_path, endpoint, sizeof(address.sun_path));
        (void)unlink(endpoint);
        if (bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 ||
            chmod(endpoint, S_IRUSR | S_IWUSR) != 0 || listen(fd, 4) != 0) {
            ERR("HD host recorder endpoint bind failed: %s", strerror(errno));
            close(fd);
            (void)unlink(endpoint);
            return;
        }

        mEndpoint = endpoint;
        mListenFd.store(fd, std::memory_order_release);
        mTimerShutdown = false;
        mTimerThread = std::thread([this] { timerLoop(); });
        mControlThread = std::thread([this] { controlLoop(); });
        INFO("HD host recorder control ready at %s", endpoint);
    }

    void stopControl() {
        const int fd = mListenFd.exchange(-1, std::memory_order_acq_rel);
        if (fd >= 0) {
            shutdown(fd, SHUT_RDWR);
            close(fd);
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
        if (!mEndpoint.empty()) {
            (void)unlink(mEndpoint.c_str());
            mEndpoint.clear();
        }
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
                    ERR("HD host recorder asynchronous stop failed: %s", error.c_str());
                }
                lock.lock();
                continue;
            }
            if (!mTimerDeadline.has_value()) {
                mTimerCondition.wait(lock, [this] {
                    return mTimerShutdown || mStopRequested || mTimerDeadline.has_value();
                });
                continue;
            }
            const auto deadline = *mTimerDeadline;
            const bool interrupted = mTimerCondition.wait_until(lock, deadline, [this, deadline] {
                return mTimerShutdown || mStopRequested || !mTimerDeadline.has_value() ||
                       *mTimerDeadline != deadline;
            });
            if (interrupted) {
                continue;
            }
            mTimerDeadline.reset();
            lock.unlock();
            const std::string error = stopRecording();
            if (!error.empty()) {
                ERR("HD host recorder automatic stop failed: %s", error.c_str());
            }
            lock.lock();
        }
    }

    void controlLoop() {
        @autoreleasepool {
            while (true) {
                const int listenFd = mListenFd.load(std::memory_order_acquire);
                if (listenFd < 0) {
                    return;
                }
                const int client = accept(listenFd, nullptr, nullptr);
                if (client < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    return;
                }
                int noSigPipe = 1;
                (void)setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe));
                handleClient(client);
                close(client);
            }
        }
    }

    void handleClient(int client) {
        RequestHeader request = {};
        if (!readFull(client, &request, sizeof(request)) || request.magic != kRequestMagic ||
            request.version != kProtocolVersion || request.pathLength > kMaxPathBytes) {
            sendResponse(client, kStatusError, "invalid HD host recorder request");
            return;
        }
        std::string path(request.pathLength, '\0');
        if (!path.empty() && !readFull(client, path.data(), path.size())) {
            sendResponse(client, kStatusError, "truncated HD host recorder request");
            return;
        }

        std::string error;
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
                if (!path.empty()) {
                    error = "stop request must not include a path";
                } else {
                    error = stopRecording();
                }
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
        sendResponse(client, error.empty() ? kStatusOk : kStatusError, error);
    }

    void sendResponse(int client, uint16_t status, const std::string& message) {
        uint64_t encodedFrames = 0;
        uint64_t droppedFrames = 0;
        bool initialStaticFrame = false;
        int initialFrameYDirection = 0;
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            encodedFrames = mEncodedFrames;
            droppedFrames = mDroppedFrames;
            initialStaticFrame = mInitialStaticFrame;
            initialFrameYDirection = mInitialFrameYDirection;
        }
        std::string responseMessage = message;
        if (status == kStatusOk) {
            char evidence[128] = {};
            snprintf(evidence, sizeof(evidence),
                     "{\"initial_static_frame\":%s,\"initial_frame_y_direction\":%d}",
                     initialStaticFrame ? "true" : "false", initialFrameYDirection);
            responseMessage = evidence;
        }
        const size_t boundedLength = std::min<size_t>(responseMessage.size(), kMaxPathBytes);
        ResponseHeader response = {
            .magic = kResponseMagic,
            .version = kProtocolVersion,
            .status = status,
            .encodedFrames = encodedFrames,
            .droppedFrames = droppedFrames,
            .messageLength = static_cast<uint32_t>(boundedLength),
        };
        if (!writeFull(client, &response, sizeof(response))) {
            return;
        }
        if (boundedLength != 0) {
            (void)writeFull(client, responseMessage.data(), boundedLength);
        }
    }

    std::string startRecording(const std::string& path, uint32_t displayId,
                               uint32_t maxDurationSeconds) {
        const uint32_t callbackDisplayId = hdPhysicalDisplayIdForScanout(displayId);
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if (mAcceptFrames || mCallbackRegistered || mWriter != nil || !mOutputPath.empty()) {
                return "one HD host recording is already active";
            }
            mOutputPath = path;
            mDisplayId = callbackDisplayId;
            mEncodedFrames = 0;
            mDroppedFrames = 0;
            mInitialStaticFrame = false;
            mInitialFrameYDirection = 0;
            mHasFinalizedRecording = false;
            mLastCompletionError.clear();
            mFrameFailure.clear();
            mStartTime = std::chrono::steady_clock::now();
            mLastFrameTime = mStartTime - std::chrono::seconds(1);
            mAcceptFrames = true;
            mCallbackRegistered = true;
            ++mGeneration;
        }

        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mAcceptFrames = false;
            mCallbackRegistered = false;
            mOutputPath.clear();
            mDisplayId = 0;
            return "gfxstream FrameBuffer is unavailable";
        }
        if (!fb->setPostCallback(&HostRecorder::onFrame, this, callbackDisplayId, true,
                                 kFramesPerSecond)) {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mAcceptFrames = false;
            mCallbackRegistered = false;
            mOutputPath.clear();
            mDisplayId = 0;
            return "gfxstream display is unavailable or already configured for recording";
        }
        const bool initialStaticFrame = fb->capturePostCallbackFrame(callbackDisplayId);
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mInitialStaticFrame = initialStaticFrame;
        }
        if (initialStaticFrame) {
            INFO("HD host recording captured the current static display frame");
        } else {
            INFO("HD host recording is waiting for the display's first posted frame");
        }
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mTimerDeadline = std::chrono::steady_clock::now() +
                             std::chrono::seconds(maxDurationSeconds);
            mTimerCondition.notify_all();
        }
        INFO("HD host recording started for scanout %u (gfxstream display %u): %s", displayId,
             callbackDisplayId, path.c_str());
        return {};
    }

    std::string stopRecording() {
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
            FrameBuffer* fb = FrameBuffer::getFB();
            if (fb) {
                fb->setPostCallback(nullptr, nullptr, displayId, true);
                if (!fb->repost()) {
                    ERR("HD host recorder could not repost display %u after recording", displayId);
                }
            }
        }

        std::lock_guard<std::mutex> lock(mStateMutex);
        if (mWriter == nil) {
            if (mOutputPath.empty()) {
                return mHasFinalizedRecording ? mLastCompletionError
                                              : "no HD host recording is active";
            }
            mOutputPath.clear();
            mDisplayId = 0;
            mHasFinalizedRecording = true;
            mLastCompletionError = mFrameFailure.empty()
                                       ? "HD host recording did not receive a frame"
                                       : std::move(mFrameFailure);
            return mLastCompletionError;
        }
        std::string timelineError;
        if (mLastPixelBuffer != nullptr) {
            const auto elapsed =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - mStartTime)
                    .count();
            const CMTime frameDuration = CMTimeMake(1, kFramesPerSecond);
            CMTime presentationTime = CMTimeMakeWithSeconds(elapsed, 600);
            if (CMTIME_IS_VALID(mLastPresentationTime) &&
                CMTimeCompare(presentationTime, mLastPresentationTime) <= 0) {
                presentationTime = CMTimeAdd(mLastPresentationTime, frameDuration);
            }

            // AVAssetWriter derives the final sample duration from the preceding timestamp
            // interval. A single terminal duplicate would therefore stretch a static frame
            // across the entire quiet period. Add an anchor one frame before the terminal
            // timestamp so the final sample duration remains one video frame while the file
            // still represents the user's complete wall-clock recording interval.
            const CMTime anchorTime = CMTimeSubtract(presentationTime, frameDuration);
            if (!CMTIME_IS_VALID(mLastPresentationTime) ||
                CMTimeCompare(anchorTime, mLastPresentationTime) > 0) {
                timelineError = appendRetainedFrameLocked(anchorTime);
            }
            if (timelineError.empty()) {
                timelineError = appendRetainedFrameLocked(presentationTime);
            }
        } else {
            timelineError = "HD host recording lost its final frame";
        }
        [mInput markAsFinished];
        dispatch_semaphore_t completed = dispatch_semaphore_create(0);
        [mWriter finishWritingWithCompletionHandler:^{ dispatch_semaphore_signal(completed); }];
        const dispatch_time_t deadline = dispatch_time(DISPATCH_TIME_NOW, 15 * NSEC_PER_SEC);
        const long timedOut = dispatch_semaphore_wait(completed, deadline);
        const AVAssetWriterStatus status = mWriter.status;
        const std::string writerError = errorDescription(mWriter.error);
        releaseWriterLocked();
        const std::string completedPath = std::move(mOutputPath);
        mOutputPath.clear();
        mDisplayId = 0;
        if (timedOut != 0) {
            mLastCompletionError = "timed out finalizing HD host recording";
        } else if (status != AVAssetWriterStatusCompleted) {
            mLastCompletionError = "HD host recording finalization failed: " + writerError;
        } else if (!timelineError.empty()) {
            mLastCompletionError = timelineError;
        } else if (chmod(completedPath.c_str(), S_IRUSR | S_IWUSR) != 0) {
            mLastCompletionError = "failed to restrict finalized HD host recording permissions: " +
                                   std::string(strerror(errno));
        } else {
            mLastCompletionError.clear();
            INFO("HD host recording finalized: %s (%llu frames, %llu dropped)",
                 completedPath.c_str(), static_cast<unsigned long long>(mEncodedFrames),
                 static_cast<unsigned long long>(mDroppedFrames));
        }
        mHasFinalizedRecording = true;
        return mLastCompletionError;
    }

    std::string appendRetainedFrameLocked(CMTime presentationTime) {
        constexpr int kReadyAttempts = 100;
        for (int attempt = 0; attempt < kReadyAttempts && !mInput.readyForMoreMediaData;
             ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (!mInput.readyForMoreMediaData) {
            ++mDroppedFrames;
            return "timed out waiting to append the HD host recording terminal frame";
        }
        if (![mAdaptor appendPixelBuffer:mLastPixelBuffer
                     withPresentationTime:presentationTime]) {
            ++mDroppedFrames;
            return "failed to append the HD host recording terminal frame";
        }
        mLastPresentationTime = presentationTime;
        ++mEncodedFrames;
        return {};
    }

    static void onFrame(void* context, uint32_t, int width, int height, int ydir, int, int,
                        unsigned char* pixels) {
        static_cast<HostRecorder*>(context)->appendFrame(width, height, ydir, pixels);
    }

    void appendFrame(int width, int height, int ydir, const unsigned char* pixels) {
        if (!pixels || width < 2 || height < 2) {
            return;
        }
        bool stopAfterFrame = false;
        uint64_t failedGeneration = 0;
        @autoreleasepool {
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
            mLastFrameTime = now;
            const int encodedWidth = width & ~1;
            const int encodedHeight = height & ~1;
            if (mWriter == nil) {
                const std::string error = createWriterLocked(encodedWidth, encodedHeight);
                if (!error.empty()) {
                    ++mDroppedFrames;
                    mAcceptFrames = false;
                    mFrameFailure = error;
                    failedGeneration = mGeneration.load(std::memory_order_acquire);
                    stopAfterFrame = true;
                    ERR("HD host recorder initialization failed: %s", error.c_str());
                }
            }
            if (!stopAfterFrame) {
                if (encodedWidth != mWidth || encodedHeight != mHeight ||
                    !mInput.readyForMoreMediaData) {
                    ++mDroppedFrames;
                    return;
                }

                CVPixelBufferRef buffer = nullptr;
                if (CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault,
                                                       mAdaptor.pixelBufferPool,
                                                       &buffer) != kCVReturnSuccess ||
                    buffer == nullptr) {
                    ++mDroppedFrames;
                    return;
                }
                CVPixelBufferLockBaseAddress(buffer, 0);
                auto* destination = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(buffer));
                const size_t destinationStride = CVPixelBufferGetBytesPerRow(buffer);
                const size_t sourceStride = static_cast<size_t>(width) * 4;
                const size_t copyBytes = static_cast<size_t>(encodedWidth) * 4;
                for (int row = 0; row < encodedHeight; ++row) {
                    const int sourceRow = ydir < 0 ? encodedHeight - 1 - row : row;
                    memcpy(destination + static_cast<size_t>(row) * destinationStride,
                           pixels + static_cast<size_t>(sourceRow) * sourceStride, copyBytes);
                }
                CVPixelBufferUnlockBaseAddress(buffer, 0);

                const auto elapsed = std::chrono::duration<double>(now - mStartTime).count();
                CMTime presentationTime = CMTimeMakeWithSeconds(elapsed, 600);
                if (CMTIME_IS_VALID(mLastPresentationTime) &&
                    CMTimeCompare(presentationTime, mLastPresentationTime) <= 0) {
                    presentationTime = CMTimeAdd(mLastPresentationTime, CMTimeMake(1, 600));
                }
                const BOOL appended = [mAdaptor appendPixelBuffer:buffer
                                             withPresentationTime:presentationTime];
                if (appended) {
                    mLastPresentationTime = presentationTime;
                    if (mLastPixelBuffer != nullptr) {
                        CVPixelBufferRelease(mLastPixelBuffer);
                    }
                    CVPixelBufferRetain(buffer);
                    mLastPixelBuffer = buffer;
                    ++mEncodedFrames;
                } else {
                    ++mDroppedFrames;
                }
                CVPixelBufferRelease(buffer);
            }
        }
        if (stopAfterFrame) {
            // The callback runs while FrameBuffer owns its posting lock. Unregistering inline
            // would deadlock, so ask the joinable lifecycle thread to stop it immediately.
            std::lock_guard<std::mutex> lock(mStateMutex);
            if (mGeneration.load(std::memory_order_acquire) == failedGeneration) {
                mStopRequested = true;
                mTimerCondition.notify_all();
            }
        }
    }

    std::string createWriterLocked(int width, int height) {
        @autoreleasepool {
            NSString* path = [NSString stringWithUTF8String:mOutputPath.c_str()];
            if (path == nil) {
                return "recording path is not valid UTF-8";
            }
            [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
            NSError* error = nil;
            mWriter = [[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:path]
                                                fileType:AVFileTypeMPEG4
                                                   error:&error];
            if (mWriter == nil) {
                return errorDescription(error);
            }
            const NSInteger bitrate = std::clamp<NSInteger>(
                static_cast<NSInteger>(width) * static_cast<NSInteger>(height) * 4, 2'000'000,
                20'000'000);
            NSDictionary* compression = @{
                AVVideoAverageBitRateKey : @(bitrate),
                AVVideoExpectedSourceFrameRateKey : @(kFramesPerSecond),
                AVVideoMaxKeyFrameIntervalKey : @(kFramesPerSecond * 2),
                AVVideoAllowFrameReorderingKey : @NO,
                AVVideoProfileLevelKey : AVVideoProfileLevelH264HighAutoLevel,
            };
            NSDictionary* encoderSpecification = @{
                (NSString*)kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder :
                    @YES,
            };
            NSDictionary* settings = @{
                AVVideoCodecKey : AVVideoCodecTypeH264,
                AVVideoWidthKey : @(width),
                AVVideoHeightKey : @(height),
                AVVideoCompressionPropertiesKey : compression,
                AVVideoEncoderSpecificationKey : encoderSpecification,
            };
            mInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo
                                                   outputSettings:settings];
            mInput.expectsMediaDataInRealTime = YES;
            NSDictionary* attributes = @{
                (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
                (NSString*)kCVPixelBufferWidthKey : @(width),
                (NSString*)kCVPixelBufferHeightKey : @(height),
                (NSString*)kCVPixelBufferIOSurfacePropertiesKey : @{},
                (NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
            };
            mAdaptor = [[AVAssetWriterInputPixelBufferAdaptor alloc]
                  initWithAssetWriterInput:mInput
                sourcePixelBufferAttributes:attributes];
            if (![mWriter canAddInput:mInput]) {
                releaseWriterLocked();
                return "AVAssetWriter rejected the HD video input";
            }
            [mWriter addInput:mInput];
            if (![mWriter startWriting]) {
                const std::string startError = errorDescription(mWriter.error);
                releaseWriterLocked();
                return startError;
            }
            if (chmod(mOutputPath.c_str(), S_IRUSR | S_IWUSR) != 0) {
                const int chmodError = errno;
                [mWriter cancelWriting];
                releaseWriterLocked();
                [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
                return "failed to restrict HD host recording permissions: " +
                       std::string(strerror(chmodError));
            }
            [mWriter startSessionAtSourceTime:kCMTimeZero];
            mWidth = width;
            mHeight = height;
            mLastPresentationTime = kCMTimeInvalid;
            return {};
        }
    }

    void releaseWriterLocked() {
        if (mLastPixelBuffer != nullptr) {
            CVPixelBufferRelease(mLastPixelBuffer);
            mLastPixelBuffer = nullptr;
        }
        [mAdaptor release];
        mAdaptor = nil;
        [mInput release];
        mInput = nil;
        [mWriter release];
        mWriter = nil;
        mWidth = 0;
        mHeight = 0;
        mLastPresentationTime = kCMTimeInvalid;
    }

    std::mutex mStateMutex;
    std::atomic<int> mListenFd{-1};
    std::thread mControlThread;
    std::thread mTimerThread;
    std::condition_variable mTimerCondition;
    std::optional<std::chrono::steady_clock::time_point> mTimerDeadline;
    bool mTimerShutdown = false;
    bool mStopRequested = false;
    std::string mEndpoint;
    std::string mOutputPath;
    uint32_t mDisplayId = 0;
    std::atomic<uint64_t> mGeneration{0};
    bool mAcceptFrames = false;
    bool mCallbackRegistered = false;
    bool mHasFinalizedRecording = false;
    std::string mLastCompletionError;
    std::string mFrameFailure;
    uint64_t mEncodedFrames = 0;
    uint64_t mDroppedFrames = 0;
    bool mInitialStaticFrame = false;
    int mInitialFrameYDirection = 0;
    int mWidth = 0;
    int mHeight = 0;
    std::chrono::steady_clock::time_point mStartTime;
    std::chrono::steady_clock::time_point mLastFrameTime;
    CMTime mLastPresentationTime = kCMTimeInvalid;
    CVPixelBufferRef mLastPixelBuffer = nullptr;
    AVAssetWriter* mWriter = nil;
    AVAssetWriterInput* mInput = nil;
    AVAssetWriterInputPixelBufferAdaptor* mAdaptor = nil;
};

}  // namespace

void startHdHostRecorderControl() { HostRecorder::get().startControl(); }

void stopHdHostRecorderControl() { HostRecorder::get().stopControl(); }

}  // namespace gfxstream
