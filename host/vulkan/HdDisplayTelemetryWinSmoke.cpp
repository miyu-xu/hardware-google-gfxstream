#include "HdDisplayTelemetry.h"

#include <windows.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

bool contains(const std::string& text, const char* expected) {
    return text.find(expected) != std::string::npos;
}

bool readUnsignedField(const std::string& text, const char* name, uint64_t* value) {
    const std::string prefix = std::string("\"") + name + "\":";
    const size_t start = text.find(prefix);
    if (start == std::string::npos) return false;
    const char* number = text.c_str() + start + prefix.size();
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(number, &end, 10);
    if (end == number) return false;
    *value = static_cast<uint64_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2 || !argv[1] || argv[1][0] == '\0') {
        std::cerr << "usage: hd_display_telemetry_windows_smoke <metrics.json>\n";
        return 2;
    }
    const std::string metricsPath(argv[1]);
    DeleteFileA(metricsPath.c_str());
    if (_putenv_s("HD_FRAME_METRICS_PATH", metricsPath.c_str()) != 0 ||
        _putenv_s("HD_FRAME_GENERATION", "7") != 0 ||
        _putenv_s("HD_GPU_STATS_PIPE", "") != 0 ||
        _putenv_s("HD_GPU_STATS_INSTANCE", "") != 0) {
        std::cerr << "failed to configure telemetry smoke environment\n";
        return 3;
    }

    gfxstream::vk::recordHdCpuReadbackBytes(4096);
    gfxstream::vk::recordHdSoftwareBlit();
    gfxstream::vk::recordHdPostWorkerFrameStart(1'000'000, 1'100'000);
    gfxstream::vk::recordHdSuccessfulPresent("fifo", 1'000'000, 11, 1080, 1920, 506, 900);
    Sleep(1100);
    gfxstream::vk::recordHdPostWorkerFrameStart(17'666'667, 17'766'667);
    gfxstream::vk::recordHdSuccessfulPresent("fifo", 1'000'000, 11, 1080, 1920, 506, 900);

    std::string payload;
    for (int attempt = 0; attempt != 100; ++attempt) {
        std::ifstream input(metricsPath, std::ios::binary);
        if (input) {
            payload.assign(std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>());
            if (!payload.empty()) break;
        }
        Sleep(20);
    }
    uint64_t queueDelayNsMax = 0;
    uint64_t workerWorkNsMax = 0;
    if (!contains(payload, "\"generation\":7") ||
        !contains(payload, "\"cpu_readback_bytes\":4096") ||
        !contains(payload, "\"software_blit_count\":1") ||
        !contains(payload, "\"cadence_probe_source_intervals\":1") ||
        !readUnsignedField(payload, "cadence_probe_post_worker_queue_delay_ns_max",
                           &queueDelayNsMax) ||
        queueDelayNsMax != 100'000 ||
        !readUnsignedField(payload, "cadence_probe_post_worker_work_ns_max", &workerWorkNsMax) ||
        workerWorkNsMax == 0 ||
        !contains(payload, "\"cadence_probe_post_worker_queue_over_33ms\":0") ||
        !contains(payload, "\"cadence_probe_post_worker_work_over_33ms\":0") ||
        !contains(payload, "\"present_mode\":\"fifo\"")) {
        std::cerr << "telemetry metrics omitted real readback, blit, cadence, worker-stage, or "
                     "present-mode accounting: "
                  << payload << "\n";
        return 4;
    }
    std::cout << "{\"status\":\"ok\",\"cpu_readback_bytes\":4096,"
                 "\"software_blit_count\":1,\"source_intervals\":1,"
                 "\"post_worker_queue_delay_ns_max\":"
              << queueDelayNsMax << ",\"post_worker_work_ns_max\":" << workerWorkNsMax << ","
                 "\"present_mode\":\"fifo\"}\n";
    return 0;
}
