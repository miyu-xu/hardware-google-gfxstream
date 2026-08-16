#ifndef HD_DISPLAY_TELEMETRY_H
#define HD_DISPLAY_TELEMETRY_H

#include <cstdint>

namespace gfxstream {
namespace vk {

// HD_VSYNC defaults to enabled. Values 0, false, off and no disable it.
bool isHdVsyncEnabled();

// Associates the selected frame currently handled by PostWorker with its host enqueue time. The
// successful present on this same worker thread consumes the values, allowing diagnostics to
// distinguish guest/source starvation from host queueing and Vulkan work without changing the
// presentation path.
void recordHdPostWorkerFrameStart(uint64_t enqueueTimestampNs,
                                  uint64_t postWorkerStartTimestampNs);

// Records a successful native vkQueuePresentKHR. Events are aggregated and handed to a background
// writer at most once per second. This call never performs pipe or filesystem I/O on the present
// thread.
void recordHdSuccessfulPresent(const char* presentMode, uint64_t hostPresentLatencyNs,
                               uint64_t cadenceProbeEpoch, uint32_t sourceWidth,
                               uint32_t sourceHeight, uint32_t swapchainWidth,
                               uint32_t swapchainHeight);

// Records actual CPU transfer or software fallback work. Normal native presentation must leave
// both counters unchanged; the explicitly bounded host recorder may increment CPU readback bytes.
void recordHdCpuReadbackBytes(uint64_t bytes);
void recordHdSoftwareBlit();

// Records swapchain lifecycle events independently from successful presents. The next metrics
// publication carries cumulative values, so a pointer-driven cadence probe can prove that a
// seemingly ordinary click or drag did not invalidate the native presentation surface.
void recordHdSwapchainRecreation(bool successful);
void recordHdSwapchainOutOfDate();

}  // namespace vk
}  // namespace gfxstream

#endif
