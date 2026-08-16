#include "HdDisplayTelemetry.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace gfxstream {
namespace vk {
namespace {

using Clock = std::chrono::steady_clock;

thread_local uint64_t sHdFrameEnqueueTimestampNs = 0;
thread_local uint64_t sHdPostWorkerQueueDelayNs = 0;
thread_local std::optional<Clock::time_point> sHdPostWorkerStart;

bool equalsIgnoreCase(const char* left, const char* right) {
    if (!left || !right) return false;
    while (*left && *right) {
        if (std::tolower(static_cast<unsigned char>(*left)) !=
            std::tolower(static_cast<unsigned char>(*right))) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

class TelemetrySink {
   public:
    TelemetrySink()
        : mEndpoint(environment("HD_GPU_STATS_PIPE")),
          mInstance(environment("HD_GPU_STATS_INSTANCE")),
          mMetricsPath(environment("HD_FRAME_METRICS_PATH")),
          mGeneration(parseGeneration()),
          mIntervalStart(Clock::now()) {
        if (telemetryEnabled() || metricsEnabled()) {
            mWriter = std::thread(&TelemetrySink::writerLoop, this);
        }
    }

    ~TelemetrySink() {
        if (mWriter.joinable()) {
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mStopping = true;
            }
            mCondition.notify_one();
            mWriter.join();
        }
        closeConnection();
    }

    void record(const char* presentMode, uint64_t hostPresentLatencyNs,
                uint64_t cadenceProbeEpoch, uint32_t sourceWidth, uint32_t sourceHeight,
                uint32_t swapchainWidth, uint32_t swapchainHeight,
                uint64_t frameEnqueueTimestampNs, uint64_t postWorkerQueueDelayNs,
                uint64_t postWorkerWorkNs) {
        if (!telemetryEnabled() && !metricsEnabled()) return;

        std::unique_lock<std::mutex> lock(mMutex);
        const auto now = Clock::now();
        ++mFrames;
        ++mTotalFrames;
        mTotalHostPresentLatencyNs += hostPresentLatencyNs;
        mTotalHostPresentLatencyNsMax =
            std::max(mTotalHostPresentLatencyNsMax, hostPresentLatencyNs);
        if (hostPresentLatencyNs > 16'666'667) ++mTotalHostPresentOver16Ms;
        if (hostPresentLatencyNs > 33'333'333) ++mTotalHostPresentOver33Ms;

        if (cadenceProbeEpoch == 0) {
            mCadenceProbeEpoch = 0;
            mCadenceProbeLastPresent.reset();
        } else {
            if (cadenceProbeEpoch != mCadenceProbeEpoch) {
                mCadenceProbeEpoch = cadenceProbeEpoch;
                mCadenceProbeFrames = 0;
                mCadenceProbeIntervalNsTotal = 0;
                mCadenceProbeIntervalNsMax = 0;
                mCadenceProbeOver33Ms = 0;
                mCadenceProbeOver50Ms = 0;
                mCadenceProbeOver100Ms = 0;
                mCadenceProbeHostPresentLatencyNsMax = 0;
                mCadenceProbeSourceIntervals = 0;
                mCadenceProbeSourceIntervalNsTotal = 0;
                mCadenceProbeSourceIntervalNsMax = 0;
                mCadenceProbeSourceOver33Ms = 0;
                mCadenceProbeSourceOver50Ms = 0;
                mCadenceProbeSourceOver100Ms = 0;
                mCadenceProbePostWorkerQueueDelayNsMax = 0;
                mCadenceProbePostWorkerQueueOver16Ms = 0;
                mCadenceProbePostWorkerQueueOver33Ms = 0;
                mCadenceProbePostWorkerWorkNsMax = 0;
                mCadenceProbePostWorkerWorkOver16Ms = 0;
                mCadenceProbePostWorkerWorkOver33Ms = 0;
                mCadenceProbeSwapchainRecreateBaseline = mSwapchainRecreateCount;
                mCadenceProbeSwapchainFailureBaseline = mSwapchainRecreateFailureCount;
                mCadenceProbeSwapchainOutOfDateBaseline = mSwapchainOutOfDateCount;
                mCadenceProbeAspectMismatchCount = 0;
                mCadenceProbeSourceExtentChangeCount = 0;
                mCadenceProbePreviousSourceWidth = 0;
                mCadenceProbePreviousSourceHeight = 0;
                mCadenceProbeLastPresent.reset();
                mCadenceProbeLastSourceEnqueueTimestampNs = 0;
            }
            ++mCadenceProbeFrames;
            mCadenceProbeHostPresentLatencyNsMax =
                std::max(mCadenceProbeHostPresentLatencyNsMax, hostPresentLatencyNs);
            mCadenceProbePostWorkerQueueDelayNsMax =
                std::max(mCadenceProbePostWorkerQueueDelayNsMax, postWorkerQueueDelayNs);
            if (postWorkerQueueDelayNs > 16'666'667) ++mCadenceProbePostWorkerQueueOver16Ms;
            if (postWorkerQueueDelayNs > 33'333'333) ++mCadenceProbePostWorkerQueueOver33Ms;
            mCadenceProbePostWorkerWorkNsMax =
                std::max(mCadenceProbePostWorkerWorkNsMax, postWorkerWorkNs);
            if (postWorkerWorkNs > 16'666'667) ++mCadenceProbePostWorkerWorkOver16Ms;
            if (postWorkerWorkNs > 33'333'333) ++mCadenceProbePostWorkerWorkOver33Ms;
            if (frameEnqueueTimestampNs != 0 &&
                mCadenceProbeLastSourceEnqueueTimestampNs != 0 &&
                frameEnqueueTimestampNs > mCadenceProbeLastSourceEnqueueTimestampNs) {
                const uint64_t sourceGapNs =
                    frameEnqueueTimestampNs - mCadenceProbeLastSourceEnqueueTimestampNs;
                ++mCadenceProbeSourceIntervals;
                mCadenceProbeSourceIntervalNsTotal += sourceGapNs;
                mCadenceProbeSourceIntervalNsMax =
                    std::max(mCadenceProbeSourceIntervalNsMax, sourceGapNs);
                if (sourceGapNs > 33'333'333) ++mCadenceProbeSourceOver33Ms;
                if (sourceGapNs > 50'000'000) ++mCadenceProbeSourceOver50Ms;
                if (sourceGapNs > 100'000'000) ++mCadenceProbeSourceOver100Ms;
            }
            if (frameEnqueueTimestampNs != 0) {
                mCadenceProbeLastSourceEnqueueTimestampNs = frameEnqueueTimestampNs;
            }
            if (mCadenceProbeLastPresent.has_value()) {
                const auto rawGap = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        now - *mCadenceProbeLastPresent)
                                        .count();
                const uint64_t gapNs = static_cast<uint64_t>(std::max<int64_t>(rawGap, 0));
                mCadenceProbeIntervalNsTotal += gapNs;
                mCadenceProbeIntervalNsMax = std::max(mCadenceProbeIntervalNsMax, gapNs);
                if (gapNs > 33'333'333) ++mCadenceProbeOver33Ms;
                if (gapNs > 50'000'000) ++mCadenceProbeOver50Ms;
                if (gapNs > 100'000'000) ++mCadenceProbeOver100Ms;
            }
            mCadenceProbeLastPresent = now;
            if (mCadenceProbePreviousSourceWidth != 0 &&
                (mCadenceProbePreviousSourceWidth != sourceWidth ||
                 mCadenceProbePreviousSourceHeight != sourceHeight)) {
                ++mCadenceProbeSourceExtentChangeCount;
            }
            mCadenceProbePreviousSourceWidth = sourceWidth;
            mCadenceProbePreviousSourceHeight = sourceHeight;
            const uint64_t sourceAcross = static_cast<uint64_t>(sourceWidth) * swapchainHeight;
            const uint64_t swapchainAcross = static_cast<uint64_t>(sourceHeight) * swapchainWidth;
            const uint64_t largerAcross = std::max(sourceAcross, swapchainAcross);
            const uint64_t aspectError = sourceAcross > swapchainAcross
                                             ? sourceAcross - swapchainAcross
                                             : swapchainAcross - sourceAcross;
            // Allow aligned guest buffers and integer viewport rounding, but never accept a
            // portrait buffer stretched into a landscape swapchain (or the reverse).
            if (sourceWidth == 0 || sourceHeight == 0 || swapchainWidth == 0 ||
                swapchainHeight == 0 || aspectError * 100 > largerAcross * 2) {
                ++mCadenceProbeAspectMismatchCount;
            }
        }
        const auto interval =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - mIntervalStart).count();
        if (interval < 1'000'000'000) return;

        const uint64_t intervalNs = static_cast<uint64_t>(interval);
        const uint64_t fpsMilli64 =
            intervalNs == 0 ? 0 : (mFrames * 1'000'000'000'000ULL) / intervalNs;
        const uint32_t fpsMilli = static_cast<uint32_t>(
            std::min<uint64_t>(fpsMilli64, std::numeric_limits<uint32_t>::max()));
        const uint64_t monotonicNs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
        // Keep only the newest interval when the writer is slow. Telemetry must never create
        // backpressure on vkQueuePresentKHR, especially while the metrics volume is full or under
        // antivirus scanning.
        mPendingSample.monotonicNs = monotonicNs;
        mPendingSample.presentedFrames = mFrames;
        mPendingSample.totalFrames = mTotalFrames;
        mPendingSample.intervalNs = intervalNs;
        mPendingSample.fpsMilli = fpsMilli;
        mPendingSample.cpuReadbackBytes = mCpuReadbackBytes;
        mPendingSample.softwareBlitCount = mSoftwareBlitCount;
        mPendingSample.hostPresentLatencyNsTotal = mTotalHostPresentLatencyNs;
        mPendingSample.hostPresentLatencyNsMax = mTotalHostPresentLatencyNsMax;
        mPendingSample.hostPresentOver16Ms = mTotalHostPresentOver16Ms;
        mPendingSample.hostPresentOver33Ms = mTotalHostPresentOver33Ms;
        mPendingSample.cadenceProbeEpoch = mCadenceProbeEpoch;
        mPendingSample.cadenceProbeFrames = mCadenceProbeFrames;
        mPendingSample.cadenceProbeIntervalNsTotal = mCadenceProbeIntervalNsTotal;
        mPendingSample.cadenceProbeIntervalNsMax = mCadenceProbeIntervalNsMax;
        mPendingSample.cadenceProbeOver33Ms = mCadenceProbeOver33Ms;
        mPendingSample.cadenceProbeOver50Ms = mCadenceProbeOver50Ms;
        mPendingSample.cadenceProbeOver100Ms = mCadenceProbeOver100Ms;
        mPendingSample.cadenceProbeHostPresentLatencyNsMax =
            mCadenceProbeHostPresentLatencyNsMax;
        mPendingSample.cadenceProbeSourceIntervals = mCadenceProbeSourceIntervals;
        mPendingSample.cadenceProbeSourceIntervalNsTotal =
            mCadenceProbeSourceIntervalNsTotal;
        mPendingSample.cadenceProbeSourceIntervalNsMax = mCadenceProbeSourceIntervalNsMax;
        mPendingSample.cadenceProbeSourceOver33Ms = mCadenceProbeSourceOver33Ms;
        mPendingSample.cadenceProbeSourceOver50Ms = mCadenceProbeSourceOver50Ms;
        mPendingSample.cadenceProbeSourceOver100Ms = mCadenceProbeSourceOver100Ms;
        mPendingSample.cadenceProbePostWorkerQueueDelayNsMax =
            mCadenceProbePostWorkerQueueDelayNsMax;
        mPendingSample.cadenceProbePostWorkerQueueOver16Ms =
            mCadenceProbePostWorkerQueueOver16Ms;
        mPendingSample.cadenceProbePostWorkerQueueOver33Ms =
            mCadenceProbePostWorkerQueueOver33Ms;
        mPendingSample.cadenceProbePostWorkerWorkNsMax = mCadenceProbePostWorkerWorkNsMax;
        mPendingSample.cadenceProbePostWorkerWorkOver16Ms =
            mCadenceProbePostWorkerWorkOver16Ms;
        mPendingSample.cadenceProbePostWorkerWorkOver33Ms =
            mCadenceProbePostWorkerWorkOver33Ms;
        mPendingSample.cadenceProbeSwapchainRecreateCount =
            mSwapchainRecreateCount - mCadenceProbeSwapchainRecreateBaseline;
        mPendingSample.cadenceProbeSwapchainFailureCount =
            mSwapchainRecreateFailureCount - mCadenceProbeSwapchainFailureBaseline;
        mPendingSample.cadenceProbeSwapchainOutOfDateCount =
            mSwapchainOutOfDateCount - mCadenceProbeSwapchainOutOfDateBaseline;
        mPendingSample.cadenceProbeAspectMismatchCount = mCadenceProbeAspectMismatchCount;
        mPendingSample.cadenceProbeSourceExtentChangeCount =
            mCadenceProbeSourceExtentChangeCount;
        mPendingSample.sourceWidth = sourceWidth;
        mPendingSample.sourceHeight = sourceHeight;
        mPendingSample.swapchainWidth = swapchainWidth;
        mPendingSample.swapchainHeight = swapchainHeight;
        mPendingSample.swapchainRecreateCount = mSwapchainRecreateCount;
        mPendingSample.swapchainRecreateFailureCount = mSwapchainRecreateFailureCount;
        mPendingSample.swapchainOutOfDateCount = mSwapchainOutOfDateCount;
        mPendingSample.presentMode = presentMode ? presentMode : "unknown";
        mSamplePending = true;
        mFrames = 0;
        mIntervalStart = now;
        lock.unlock();
        mCondition.notify_one();
    }

    void recordSwapchainRecreation(bool successful) {
        if (!telemetryEnabled() && !metricsEnabled()) return;
        std::lock_guard<std::mutex> lock(mMutex);
        if (successful) {
            ++mSwapchainRecreateCount;
        } else {
            ++mSwapchainRecreateFailureCount;
        }
    }

    void recordSwapchainOutOfDate() {
        if (!telemetryEnabled() && !metricsEnabled()) return;
        std::lock_guard<std::mutex> lock(mMutex);
        ++mSwapchainOutOfDateCount;
    }

    void recordCpuReadbackBytes(uint64_t bytes) {
        if (bytes == 0 || (!telemetryEnabled() && !metricsEnabled())) return;
        std::lock_guard<std::mutex> lock(mMutex);
        mCpuReadbackBytes =
            bytes > std::numeric_limits<uint64_t>::max() - mCpuReadbackBytes
                ? std::numeric_limits<uint64_t>::max()
                : mCpuReadbackBytes + bytes;
    }

    void recordSoftwareBlit() {
        if (!telemetryEnabled() && !metricsEnabled()) return;
        std::lock_guard<std::mutex> lock(mMutex);
        if (mSoftwareBlitCount != std::numeric_limits<uint64_t>::max()) {
            ++mSoftwareBlitCount;
        }
    }

   private:
    struct Sample {
        uint64_t monotonicNs = 0;
        uint64_t presentedFrames = 0;
        uint64_t totalFrames = 0;
        uint64_t intervalNs = 0;
        uint32_t fpsMilli = 0;
        uint64_t cpuReadbackBytes = 0;
        uint64_t softwareBlitCount = 0;
        uint64_t hostPresentLatencyNsTotal = 0;
        uint64_t hostPresentLatencyNsMax = 0;
        uint64_t hostPresentOver16Ms = 0;
        uint64_t hostPresentOver33Ms = 0;
        uint64_t cadenceProbeEpoch = 0;
        uint64_t cadenceProbeFrames = 0;
        uint64_t cadenceProbeIntervalNsTotal = 0;
        uint64_t cadenceProbeIntervalNsMax = 0;
        uint64_t cadenceProbeOver33Ms = 0;
        uint64_t cadenceProbeOver50Ms = 0;
        uint64_t cadenceProbeOver100Ms = 0;
        uint64_t cadenceProbeHostPresentLatencyNsMax = 0;
        uint64_t cadenceProbeSourceIntervals = 0;
        uint64_t cadenceProbeSourceIntervalNsTotal = 0;
        uint64_t cadenceProbeSourceIntervalNsMax = 0;
        uint64_t cadenceProbeSourceOver33Ms = 0;
        uint64_t cadenceProbeSourceOver50Ms = 0;
        uint64_t cadenceProbeSourceOver100Ms = 0;
        uint64_t cadenceProbePostWorkerQueueDelayNsMax = 0;
        uint64_t cadenceProbePostWorkerQueueOver16Ms = 0;
        uint64_t cadenceProbePostWorkerQueueOver33Ms = 0;
        uint64_t cadenceProbePostWorkerWorkNsMax = 0;
        uint64_t cadenceProbePostWorkerWorkOver16Ms = 0;
        uint64_t cadenceProbePostWorkerWorkOver33Ms = 0;
        uint64_t cadenceProbeSwapchainRecreateCount = 0;
        uint64_t cadenceProbeSwapchainFailureCount = 0;
        uint64_t cadenceProbeSwapchainOutOfDateCount = 0;
        uint64_t cadenceProbeAspectMismatchCount = 0;
        uint64_t cadenceProbeSourceExtentChangeCount = 0;
        uint32_t sourceWidth = 0;
        uint32_t sourceHeight = 0;
        uint32_t swapchainWidth = 0;
        uint32_t swapchainHeight = 0;
        uint64_t swapchainRecreateCount = 0;
        uint64_t swapchainRecreateFailureCount = 0;
        uint64_t swapchainOutOfDateCount = 0;
        std::string presentMode;
    };

    static std::string environment(const char* name) {
        const char* value = std::getenv(name);
        return value ? value : "";
    }

    static uint64_t parseGeneration() {
        const char* value = std::getenv("HD_FRAME_GENERATION");
        if (!value || value[0] == '\0') return 0;
        char* end = nullptr;
        const unsigned long long parsed = std::strtoull(value, &end, 10);
        return end != value && end && end[0] == '\0' ? static_cast<uint64_t>(parsed) : 0;
    }

    bool telemetryEnabled() const { return !mEndpoint.empty() && !mInstance.empty(); }

    bool metricsEnabled() const { return !mMetricsPath.empty() && mGeneration != 0; }

    void writerLoop() {
        for (;;) {
            Sample sample;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCondition.wait(lock, [this] { return mStopping || mSamplePending; });
                if (!mSamplePending && mStopping) return;
                sample = std::move(mPendingSample);
                mSamplePending = false;
            }

            if (telemetryEnabled()) {
                char payload[2304];
                const int size = std::snprintf(
                    payload, sizeof(payload),
                    "{\"protocol_version\":1,\"instance_id\":\"%s\",\"monotonic_ns\":%llu,"
                    "\"presented_frames\":%llu,\"interval_ns\":%llu,\"fps_milli\":%u,"
                    "\"cpu_readback_bytes\":%llu,\"software_blit_count\":%llu,"
                    "\"host_present_latency_ns_total\":%llu,"
                    "\"host_present_latency_ns_max\":%llu,"
                    "\"host_present_over_16ms\":%llu,\"host_present_over_33ms\":%llu,"
                    "\"cadence_probe_epoch\":%llu,\"cadence_probe_frames\":%llu,"
                    "\"cadence_probe_interval_ns_total\":%llu,"
                    "\"cadence_probe_interval_ns_max\":%llu,"
                    "\"cadence_probe_over_33ms\":%llu,\"cadence_probe_over_50ms\":%llu,"
                    "\"cadence_probe_over_100ms\":%llu,"
                    "\"cadence_probe_host_present_latency_ns_max\":%llu,"
                    "\"cadence_probe_source_intervals\":%llu,"
                    "\"cadence_probe_source_interval_ns_total\":%llu,"
                    "\"cadence_probe_source_interval_ns_max\":%llu,"
                    "\"cadence_probe_source_over_33ms\":%llu,"
                    "\"cadence_probe_source_over_50ms\":%llu,"
                    "\"cadence_probe_source_over_100ms\":%llu,"
                    "\"cadence_probe_post_worker_queue_delay_ns_max\":%llu,"
                    "\"cadence_probe_post_worker_queue_over_16ms\":%llu,"
                    "\"cadence_probe_post_worker_queue_over_33ms\":%llu,"
                    "\"cadence_probe_post_worker_work_ns_max\":%llu,"
                    "\"cadence_probe_post_worker_work_over_16ms\":%llu,"
                    "\"cadence_probe_post_worker_work_over_33ms\":%llu,"
                    "\"cadence_probe_swapchain_recreate_count\":%llu,"
                    "\"cadence_probe_swapchain_failure_count\":%llu,"
                    "\"cadence_probe_swapchain_out_of_date_count\":%llu,"
                    "\"cadence_probe_aspect_mismatch_count\":%llu,"
                    "\"cadence_probe_source_extent_change_count\":%llu,"
                    "\"source_width\":%u,\"source_height\":%u,"
                    "\"swapchain_width\":%u,\"swapchain_height\":%u,"
                    "\"present_mode\":\"%s\"}\n",
                    mInstance.c_str(), static_cast<unsigned long long>(sample.monotonicNs),
                    static_cast<unsigned long long>(sample.presentedFrames),
                    static_cast<unsigned long long>(sample.intervalNs), sample.fpsMilli,
                    static_cast<unsigned long long>(sample.cpuReadbackBytes),
                    static_cast<unsigned long long>(sample.softwareBlitCount),
                    static_cast<unsigned long long>(sample.hostPresentLatencyNsTotal),
                    static_cast<unsigned long long>(sample.hostPresentLatencyNsMax),
                    static_cast<unsigned long long>(sample.hostPresentOver16Ms),
                    static_cast<unsigned long long>(sample.hostPresentOver33Ms),
                    static_cast<unsigned long long>(sample.cadenceProbeEpoch),
                    static_cast<unsigned long long>(sample.cadenceProbeFrames),
                    static_cast<unsigned long long>(sample.cadenceProbeIntervalNsTotal),
                    static_cast<unsigned long long>(sample.cadenceProbeIntervalNsMax),
                    static_cast<unsigned long long>(sample.cadenceProbeOver33Ms),
                    static_cast<unsigned long long>(sample.cadenceProbeOver50Ms),
                    static_cast<unsigned long long>(sample.cadenceProbeOver100Ms),
                    static_cast<unsigned long long>(
                        sample.cadenceProbeHostPresentLatencyNsMax),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceIntervals),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceIntervalNsTotal),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceIntervalNsMax),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceOver33Ms),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceOver50Ms),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceOver100Ms),
                    static_cast<unsigned long long>(
                        sample.cadenceProbePostWorkerQueueDelayNsMax),
                    static_cast<unsigned long long>(
                        sample.cadenceProbePostWorkerQueueOver16Ms),
                    static_cast<unsigned long long>(
                        sample.cadenceProbePostWorkerQueueOver33Ms),
                    static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkNsMax),
                    static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkOver16Ms),
                    static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkOver33Ms),
                    static_cast<unsigned long long>(
                        sample.cadenceProbeSwapchainRecreateCount),
                    static_cast<unsigned long long>(
                        sample.cadenceProbeSwapchainFailureCount),
                    static_cast<unsigned long long>(
                        sample.cadenceProbeSwapchainOutOfDateCount),
                    static_cast<unsigned long long>(sample.cadenceProbeAspectMismatchCount),
                    static_cast<unsigned long long>(sample.cadenceProbeSourceExtentChangeCount),
                    sample.sourceWidth, sample.sourceHeight, sample.swapchainWidth,
                    sample.swapchainHeight,
                    sample.presentMode.c_str());
                if (size > 0 && static_cast<size_t>(size) < sizeof(payload)) {
                    writePayload(payload, static_cast<size_t>(size));
                }
            }
            if (metricsEnabled()) {
                publishMetricsSample(sample);
            }

            std::lock_guard<std::mutex> lock(mMutex);
            if (!mSamplePending && mStopping) return;
        }
    }

    void publishMetricsSample(const Sample& sample) {
        if (publishMetrics(sample)) {
            mMetricsPublishFailureReported = false;
            return;
        }
        if (!mMetricsPublishFailureReported) {
            std::fprintf(stderr, "HD frame metrics atomic publish failed for %s; will retry\n",
                         mMetricsPath.c_str());
            std::fflush(stderr);
            mMetricsPublishFailureReported = true;
        }
    }

    bool publishMetrics(const Sample& sample) const {
#ifdef _WIN32
        MEMORYSTATUSEX memoryStatus = {};
        memoryStatus.dwLength = sizeof(memoryStatus);
        const bool memoryStatusAvailable = GlobalMemoryStatusEx(&memoryStatus) != FALSE;
        const uint64_t hostAvailableMemoryBytes =
            memoryStatusAvailable ? memoryStatus.ullAvailPhys : 0;
        const uint32_t hostMemoryLoadPercent =
            memoryStatusAvailable ? memoryStatus.dwMemoryLoad : 0;
        const std::string temporary =
            mMetricsPath + ".tmp." + std::to_string(GetCurrentProcessId());
        FILE* file = nullptr;
        if (fopen_s(&file, temporary.c_str(), "wb") != 0 || !file) return false;
        const int result = std::fprintf(
            file,
            "{\"generation\":%llu,\"produced_frames\":%llu,\"imported_frames\":%llu,"
            "\"presented_frames\":%llu,\"dropped_frames\":0,"
            "\"cpu_readback_bytes\":%llu,\"software_blit_count\":%llu,\"fps_milli\":%u,"
            "\"host_present_latency_ns_total\":%llu,"
            "\"host_present_latency_ns_max\":%llu,"
            "\"host_present_over_16ms\":%llu,\"host_present_over_33ms\":%llu,"
            "\"cadence_probe_epoch\":%llu,\"cadence_probe_frames\":%llu,"
            "\"cadence_probe_interval_ns_total\":%llu,"
            "\"cadence_probe_interval_ns_max\":%llu,"
            "\"cadence_probe_over_33ms\":%llu,\"cadence_probe_over_50ms\":%llu,"
            "\"cadence_probe_over_100ms\":%llu,"
            "\"cadence_probe_host_present_latency_ns_max\":%llu,"
            "\"cadence_probe_source_intervals\":%llu,"
            "\"cadence_probe_source_interval_ns_total\":%llu,"
            "\"cadence_probe_source_interval_ns_max\":%llu,"
            "\"cadence_probe_source_over_33ms\":%llu,"
            "\"cadence_probe_source_over_50ms\":%llu,"
            "\"cadence_probe_source_over_100ms\":%llu,"
            "\"cadence_probe_post_worker_queue_delay_ns_max\":%llu,"
            "\"cadence_probe_post_worker_queue_over_16ms\":%llu,"
            "\"cadence_probe_post_worker_queue_over_33ms\":%llu,"
            "\"cadence_probe_post_worker_work_ns_max\":%llu,"
            "\"cadence_probe_post_worker_work_over_16ms\":%llu,"
            "\"cadence_probe_post_worker_work_over_33ms\":%llu,"
            "\"cadence_probe_swapchain_recreate_count\":%llu,"
            "\"cadence_probe_swapchain_failure_count\":%llu,"
            "\"cadence_probe_swapchain_out_of_date_count\":%llu,"
            "\"cadence_probe_aspect_mismatch_count\":%llu,"
            "\"cadence_probe_source_extent_change_count\":%llu,"
            "\"source_width\":%u,\"source_height\":%u,"
            "\"swapchain_width\":%u,\"swapchain_height\":%u,"
            "\"host_available_memory_bytes\":%llu,"
            "\"host_memory_load_percent\":%u,"
            "\"swapchain_recreate_count\":%llu,"
            "\"swapchain_recreate_failure_count\":%llu,"
            "\"swapchain_out_of_date_count\":%llu,"
            "\"present_mode\":\"%s\"}\n",
            static_cast<unsigned long long>(mGeneration),
            static_cast<unsigned long long>(sample.totalFrames),
            static_cast<unsigned long long>(sample.totalFrames),
            static_cast<unsigned long long>(sample.totalFrames),
            static_cast<unsigned long long>(sample.cpuReadbackBytes),
            static_cast<unsigned long long>(sample.softwareBlitCount), sample.fpsMilli,
            static_cast<unsigned long long>(sample.hostPresentLatencyNsTotal),
            static_cast<unsigned long long>(sample.hostPresentLatencyNsMax),
            static_cast<unsigned long long>(sample.hostPresentOver16Ms),
            static_cast<unsigned long long>(sample.hostPresentOver33Ms),
            static_cast<unsigned long long>(sample.cadenceProbeEpoch),
            static_cast<unsigned long long>(sample.cadenceProbeFrames),
            static_cast<unsigned long long>(sample.cadenceProbeIntervalNsTotal),
            static_cast<unsigned long long>(sample.cadenceProbeIntervalNsMax),
            static_cast<unsigned long long>(sample.cadenceProbeOver33Ms),
            static_cast<unsigned long long>(sample.cadenceProbeOver50Ms),
            static_cast<unsigned long long>(sample.cadenceProbeOver100Ms),
            static_cast<unsigned long long>(sample.cadenceProbeHostPresentLatencyNsMax),
            static_cast<unsigned long long>(sample.cadenceProbeSourceIntervals),
            static_cast<unsigned long long>(sample.cadenceProbeSourceIntervalNsTotal),
            static_cast<unsigned long long>(sample.cadenceProbeSourceIntervalNsMax),
            static_cast<unsigned long long>(sample.cadenceProbeSourceOver33Ms),
            static_cast<unsigned long long>(sample.cadenceProbeSourceOver50Ms),
            static_cast<unsigned long long>(sample.cadenceProbeSourceOver100Ms),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerQueueDelayNsMax),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerQueueOver16Ms),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerQueueOver33Ms),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkNsMax),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkOver16Ms),
            static_cast<unsigned long long>(sample.cadenceProbePostWorkerWorkOver33Ms),
            static_cast<unsigned long long>(sample.cadenceProbeSwapchainRecreateCount),
            static_cast<unsigned long long>(sample.cadenceProbeSwapchainFailureCount),
            static_cast<unsigned long long>(sample.cadenceProbeSwapchainOutOfDateCount),
            static_cast<unsigned long long>(sample.cadenceProbeAspectMismatchCount),
            static_cast<unsigned long long>(sample.cadenceProbeSourceExtentChangeCount),
            sample.sourceWidth, sample.sourceHeight, sample.swapchainWidth,
            sample.swapchainHeight,
            static_cast<unsigned long long>(hostAvailableMemoryBytes),
            hostMemoryLoadPercent,
            static_cast<unsigned long long>(sample.swapchainRecreateCount),
            static_cast<unsigned long long>(sample.swapchainRecreateFailureCount),
            static_cast<unsigned long long>(sample.swapchainOutOfDateCount),
            sample.presentMode.c_str());
        bool complete = result > 0;
        if (std::fflush(file) != 0) complete = false;
        if (std::fclose(file) != 0) complete = false;
        if (!complete) {
            (void)DeleteFileA(temporary.c_str());
            return false;
        }
        for (int attempt = 0; attempt != 4; ++attempt) {
            if (MoveFileExA(temporary.c_str(), mMetricsPath.c_str(),
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                return true;
            }
            const DWORD error = GetLastError();
            if (error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED) break;
            Sleep(1);
        }
        (void)DeleteFileA(temporary.c_str());
        return false;
#else
        (void)sample;
        return true;
#endif
    }

    bool connectIfNeeded() {
#ifdef _WIN32
        if (mPipe != INVALID_HANDLE_VALUE) return true;
        mPipe = CreateFileA(mEndpoint.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (mPipe == INVALID_HANDLE_VALUE) {
            reportConnectionFailure("CreateFile", static_cast<int>(GetLastError()));
            return false;
        }
        mConnectionFailureReported = false;
        return true;
#else
        if (mSocket >= 0) return true;
        sockaddr_un address = {};
        if (mEndpoint.size() >= sizeof(address.sun_path)) {
            reportConnectionFailure("endpoint-too-long", 0);
            return false;
        }
        const int socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socketFd < 0) {
            reportConnectionFailure("socket", errno);
            return false;
        }
        address.sun_family = AF_UNIX;
        std::memcpy(address.sun_path, mEndpoint.c_str(), mEndpoint.size() + 1);
        if (connect(socketFd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
            const int savedErrno = errno;
            close(socketFd);
            reportConnectionFailure("connect", savedErrno);
            return false;
        }
        mSocket = socketFd;
        mConnectionFailureReported = false;
        return true;
#endif
    }

    void writePayload(const char* payload, size_t size) {
        if (!connectIfNeeded()) return;
#ifdef _WIN32
        size_t offset = 0;
        while (offset < size) {
            DWORD written = 0;
            const auto remaining = static_cast<DWORD>(size - offset);
            if (!WriteFile(mPipe, payload + offset, remaining, &written, nullptr) || written == 0) {
                reportConnectionFailure("WriteFile", static_cast<int>(GetLastError()));
                closeConnection();
                return;
            }
            offset += static_cast<size_t>(written);
        }
#else
#ifdef MSG_NOSIGNAL
        constexpr int kSendFlags = MSG_NOSIGNAL;
#else
        constexpr int kSendFlags = 0;
#endif
        size_t offset = 0;
        while (offset < size) {
            const auto written = send(mSocket, payload + offset, size - offset, kSendFlags);
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) {
                reportConnectionFailure("send", errno);
                closeConnection();
                return;
            }
            offset += static_cast<size_t>(written);
        }
#endif
    }

    void reportConnectionFailure(const char* operation, int error) {
        if (mConnectionFailureReported) return;
        std::fprintf(stderr, "HD GPU telemetry %s failed for %s (error=%d); will retry\n",
                     operation, mEndpoint.c_str(), error);
        std::fflush(stderr);
        mConnectionFailureReported = true;
    }

    void closeConnection() {
#ifdef _WIN32
        if (mPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(mPipe);
            mPipe = INVALID_HANDLE_VALUE;
        }
#else
        if (mSocket >= 0) {
            close(mSocket);
            mSocket = -1;
        }
#endif
    }

    std::mutex mMutex;
    std::condition_variable mCondition;
    const std::string mEndpoint;
    const std::string mInstance;
    const std::string mMetricsPath;
    const uint64_t mGeneration;
    Clock::time_point mIntervalStart;
    uint64_t mFrames = 0;
    uint64_t mTotalFrames = 0;
    uint64_t mCpuReadbackBytes = 0;
    uint64_t mSoftwareBlitCount = 0;
    uint64_t mTotalHostPresentLatencyNs = 0;
    uint64_t mTotalHostPresentLatencyNsMax = 0;
    uint64_t mTotalHostPresentOver16Ms = 0;
    uint64_t mTotalHostPresentOver33Ms = 0;
    uint64_t mCadenceProbeEpoch = 0;
    uint64_t mCadenceProbeFrames = 0;
    uint64_t mCadenceProbeIntervalNsTotal = 0;
    uint64_t mCadenceProbeIntervalNsMax = 0;
    uint64_t mCadenceProbeOver33Ms = 0;
    uint64_t mCadenceProbeOver50Ms = 0;
    uint64_t mCadenceProbeOver100Ms = 0;
    uint64_t mCadenceProbeHostPresentLatencyNsMax = 0;
    uint64_t mCadenceProbeSourceIntervals = 0;
    uint64_t mCadenceProbeSourceIntervalNsTotal = 0;
    uint64_t mCadenceProbeSourceIntervalNsMax = 0;
    uint64_t mCadenceProbeSourceOver33Ms = 0;
    uint64_t mCadenceProbeSourceOver50Ms = 0;
    uint64_t mCadenceProbeSourceOver100Ms = 0;
    uint64_t mCadenceProbePostWorkerQueueDelayNsMax = 0;
    uint64_t mCadenceProbePostWorkerQueueOver16Ms = 0;
    uint64_t mCadenceProbePostWorkerQueueOver33Ms = 0;
    uint64_t mCadenceProbePostWorkerWorkNsMax = 0;
    uint64_t mCadenceProbePostWorkerWorkOver16Ms = 0;
    uint64_t mCadenceProbePostWorkerWorkOver33Ms = 0;
    uint64_t mCadenceProbeLastSourceEnqueueTimestampNs = 0;
    uint64_t mCadenceProbeSwapchainRecreateBaseline = 0;
    uint64_t mCadenceProbeSwapchainFailureBaseline = 0;
    uint64_t mCadenceProbeSwapchainOutOfDateBaseline = 0;
    uint64_t mCadenceProbeAspectMismatchCount = 0;
    uint64_t mCadenceProbeSourceExtentChangeCount = 0;
    uint32_t mCadenceProbePreviousSourceWidth = 0;
    uint32_t mCadenceProbePreviousSourceHeight = 0;
    std::optional<Clock::time_point> mCadenceProbeLastPresent;
    uint64_t mSwapchainRecreateCount = 0;
    uint64_t mSwapchainRecreateFailureCount = 0;
    uint64_t mSwapchainOutOfDateCount = 0;
    Sample mPendingSample;
    bool mSamplePending = false;
    bool mStopping = false;
    std::thread mWriter;
    bool mMetricsPublishFailureReported = false;
    bool mConnectionFailureReported = false;
#ifdef _WIN32
    HANDLE mPipe = INVALID_HANDLE_VALUE;
#else
    int mSocket = -1;
#endif
};

TelemetrySink& telemetrySink() {
    static TelemetrySink sink;
    return sink;
}

}  // namespace

bool isHdVsyncEnabled() {
    const char* value = std::getenv("HD_VSYNC");
    if (!value || !*value) return true;
    return !(equalsIgnoreCase(value, "0") || equalsIgnoreCase(value, "false") ||
             equalsIgnoreCase(value, "off") || equalsIgnoreCase(value, "no"));
}

void recordHdPostWorkerFrameStart(uint64_t enqueueTimestampNs,
                                  uint64_t postWorkerStartTimestampNs) {
    sHdFrameEnqueueTimestampNs = enqueueTimestampNs;
    sHdPostWorkerQueueDelayNs =
        enqueueTimestampNs != 0 && postWorkerStartTimestampNs >= enqueueTimestampNs
            ? postWorkerStartTimestampNs - enqueueTimestampNs
            : 0;
    sHdPostWorkerStart = Clock::now();
}

void recordHdSuccessfulPresent(const char* presentMode, uint64_t hostPresentLatencyNs,
                               uint64_t cadenceProbeEpoch, uint32_t sourceWidth,
                               uint32_t sourceHeight, uint32_t swapchainWidth,
                               uint32_t swapchainHeight) {
    const auto now = Clock::now();
    const uint64_t postWorkerWorkNs = sHdPostWorkerStart.has_value()
                                          ? static_cast<uint64_t>(std::max<int64_t>(
                                                std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                    now - *sHdPostWorkerStart)
                                                    .count(),
                                                0))
                                          : 0;
    telemetrySink().record(presentMode, hostPresentLatencyNs, cadenceProbeEpoch, sourceWidth,
                           sourceHeight, swapchainWidth, swapchainHeight,
                           sHdFrameEnqueueTimestampNs, sHdPostWorkerQueueDelayNs,
                           postWorkerWorkNs);
    sHdFrameEnqueueTimestampNs = 0;
    sHdPostWorkerQueueDelayNs = 0;
    sHdPostWorkerStart.reset();
}

void recordHdCpuReadbackBytes(uint64_t bytes) {
    telemetrySink().recordCpuReadbackBytes(bytes);
}

void recordHdSoftwareBlit() {
    telemetrySink().recordSoftwareBlit();
}

void recordHdSwapchainRecreation(bool successful) {
    telemetrySink().recordSwapchainRecreation(successful);
}

void recordHdSwapchainOutOfDate() {
    telemetrySink().recordSwapchainOutOfDate();
}

}  // namespace vk
}  // namespace gfxstream
