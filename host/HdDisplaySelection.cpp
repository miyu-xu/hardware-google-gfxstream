// Copyright 2026 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include "HdDisplaySelection.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>

namespace gfxstream {
namespace {

std::atomic<uint32_t> sSelectedScanout{0};
std::array<std::atomic<uint32_t>, kHdMaxScanouts * kHdResourcesPerScanout> sResources{};
std::array<std::atomic<uint32_t>, kHdMaxScanouts> sResourceCursors{};
std::array<std::atomic<uint32_t>, kHdMaxScanouts> sLatestResources{};
std::array<std::atomic<uint64_t>, kHdMaxScanouts> sScanoutSizes{};
std::array<std::atomic<uintptr_t>, kHdMaxScanouts> sAuthoritativeViewportWindows{};
std::array<std::atomic<uint64_t>, kHdMaxScanouts> sAuthoritativeViewportSizes{};

uint64_t packedScanoutSize(uint32_t width, uint32_t height) {
    return (static_cast<uint64_t>(width) << 32) | height;
}

bool configuredSizeMatches(uint64_t packed, uint32_t width, uint32_t height) {
    if (packed == 0) {
        return false;
    }
    const uint32_t configuredWidth = static_cast<uint32_t>(packed >> 32);
    const uint32_t configuredHeight = static_cast<uint32_t>(packed);
    const auto alignedMatch = [](uint32_t actual, uint32_t configured) {
        return actual >= configured && actual - configured <= 64;
    };
    return (alignedMatch(width, configuredWidth) && alignedMatch(height, configuredHeight)) ||
           (alignedMatch(width, configuredHeight) && alignedMatch(height, configuredWidth));
}

}  // namespace

bool hdDisplaySelectionEnabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("HD_DISPLAY_SELECTION");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}

uint32_t hdSelectedScanout() {
    return sSelectedScanout.load(std::memory_order_acquire);
}

uint32_t hdSelectedDisplay() {
    return hdPhysicalDisplayIdForScanout(hdSelectedScanout());
}

void hdSelectDisplay(uint32_t scanoutId) {
    if (scanoutId < kHdMaxScanouts) {
        sSelectedScanout.store(scanoutId, std::memory_order_release);
    }
}

void hdConfigureScanoutSize(uint32_t scanoutId, uint32_t width, uint32_t height) {
    if (scanoutId < kHdMaxScanouts) {
        sScanoutSizes[scanoutId].store(packedScanoutSize(width, height),
                                       std::memory_order_release);
    }
}

void hdSetAuthoritativeHostViewport(uint32_t scanoutId, uintptr_t nativeWindowHandle,
                                    uint32_t width, uint32_t height) {
    if (scanoutId >= kHdMaxScanouts || nativeWindowHandle == 0 || width == 0 || height == 0) {
        return;
    }
    // Publish dimensions before the window identity. The acquire load in the setup path then sees
    // one complete viewport record without requiring a mutex in crosvm's presentation callback.
    sAuthoritativeViewportSizes[scanoutId].store(packedScanoutSize(width, height),
                                                  std::memory_order_relaxed);
    sAuthoritativeViewportWindows[scanoutId].store(nativeWindowHandle,
                                                    std::memory_order_release);
}

bool hdHasAuthoritativeHostViewport(uint32_t scanoutId, uintptr_t nativeWindowHandle) {
    if (scanoutId >= kHdMaxScanouts || nativeWindowHandle == 0 ||
        sAuthoritativeViewportWindows[scanoutId].load(std::memory_order_acquire) !=
            nativeWindowHandle) {
        return false;
    }
    return sAuthoritativeViewportSizes[scanoutId].load(std::memory_order_relaxed) != 0;
}

uint32_t hdScanoutForDisplayBuffer(uint32_t displayId, uint32_t width, uint32_t height) {
    if (displayId < kHdMaxScanouts &&
        configuredSizeMatches(sScanoutSizes[displayId].load(std::memory_order_acquire), width,
                              height)) {
        return displayId;
    }

    uint32_t match = kHdInvalidScanout;
    for (uint32_t scanoutId = 0; scanoutId < kHdMaxScanouts; ++scanoutId) {
        if (!configuredSizeMatches(sScanoutSizes[scanoutId].load(std::memory_order_acquire), width,
                                   height)) {
            continue;
        }
        if (match != kHdInvalidScanout) {
            return kHdInvalidScanout;
        }
        match = scanoutId;
    }
    return match;
}

uint32_t hdLatestScanoutResource(uint32_t scanoutId) {
    return scanoutId < kHdMaxScanouts
               ? sLatestResources[scanoutId].load(std::memory_order_acquire)
               : 0;
}

void hdSetScanoutResource(uint32_t scanoutId, uint32_t resourceId) {
    if (scanoutId >= kHdMaxScanouts) {
        return;
    }
    const std::size_t begin = scanoutId * kHdResourcesPerScanout;
    if (resourceId == 0) {
        // SET_SCANOUT(0) disables the guest's active scanout association, but it does not mean
        // the native window should forget its last valid frame. HD can temporarily detach and
        // reattach the same crosvm HWND during layout, maximize, sidebar, and display switches.
        // Keep the retained resource so selectDisplayForHd() can synchronously restore that frame;
        // the repost path revalidates the handle and fails safely if the ColorBuffer was destroyed.
        for (std::size_t slot = begin; slot < begin + kHdResourcesPerScanout; ++slot) {
            sResources[slot].store(0, std::memory_order_release);
        }
        return;
    }

    sLatestResources[scanoutId].store(resourceId, std::memory_order_release);

    bool alreadyOnTarget = false;
    for (std::size_t slot = 0; slot < sResources.size(); ++slot) {
        auto& resource = sResources[slot];
        uint32_t current = resource.load(std::memory_order_acquire);
        if (current == resourceId) {
            if (slot >= begin && slot < begin + kHdResourcesPerScanout) {
                alreadyOnTarget = true;
            } else {
                resource.compare_exchange_strong(current, 0, std::memory_order_acq_rel);
            }
        }
    }
    if (alreadyOnTarget) {
        return;
    }
    const uint32_t cursor =
        sResourceCursors[scanoutId].fetch_add(1, std::memory_order_relaxed);
    sResources[begin + cursor % kHdResourcesPerScanout].store(resourceId,
                                                               std::memory_order_release);
}

uint32_t hdScanoutForResource(uint32_t resourceId) {
    if (resourceId == 0) {
        return kHdInvalidScanout;
    }
    for (uint32_t scanoutId = 0; scanoutId < kHdMaxScanouts; ++scanoutId) {
        const std::size_t begin = scanoutId * kHdResourcesPerScanout;
        for (std::size_t slot = begin; slot < begin + kHdResourcesPerScanout; ++slot) {
            if (sResources[slot].load(std::memory_order_acquire) == resourceId) {
                return scanoutId;
            }
        }
    }
    return kHdInvalidScanout;
}

bool hdShouldPresentResource(uint32_t resourceId) {
    const uint32_t scanoutId = hdScanoutForResource(resourceId);
    return scanoutId != kHdInvalidScanout && scanoutId == hdSelectedScanout();
}

}  // namespace gfxstream
