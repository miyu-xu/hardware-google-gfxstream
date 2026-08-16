// Copyright 2026 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
#pragma once

#include <cstdint>
#include <limits>

namespace gfxstream {

bool hdDisplaySelectionEnabled();

inline constexpr uint32_t kHdMaxScanouts = 16;
// SurfaceFlinger normally rotates through three client-target buffers, but display reconfiguration
// can leave more buffers in flight while their asynchronous flush callbacks complete. Remember a
// bounded history per scanout instead of only its latest resource. Otherwise every callback except
// the newest one is rejected, which makes the native Player alternate between stale, distorted and
// black frames.
inline constexpr uint32_t kHdResourcesPerScanout = 16;
inline constexpr uint32_t kHdInvalidScanout = std::numeric_limits<uint32_t>::max();

uint32_t hdSelectedScanout();

// Crosvm scanout numbering matches the HWC display/port id carried by gfxstream's
// ComposeDevice_v2 multi-display registry. Android assigns a separate framework logical display
// id (for example, logical display 2 for HWC display 1); that framework id must not be used here.
inline uint32_t hdPhysicalDisplayIdForScanout(uint32_t scanoutId) {
    return scanoutId;
}

uint32_t hdSelectedDisplay();
void hdSelectDisplay(uint32_t scanoutId);
void hdConfigureScanoutSize(uint32_t scanoutId, uint32_t width, uint32_t height);
// HD's host viewport is authoritative for the native Player surface. Crosvm can replay its
// ordinary startup projection after that transaction completes; remember the exact parent HWND so
// the replay cannot roll the swapchain back to the default 1024x768 extent. A different HWND is a
// new surface and must still be allowed to initialize before HD attaches it.
void hdSetAuthoritativeHostViewport(uint32_t scanoutId, uintptr_t nativeWindowHandle,
                                    uint32_t width, uint32_t height);
bool hdHasAuthoritativeHostViewport(uint32_t scanoutId, uintptr_t nativeWindowHandle);
uint32_t hdScanoutForDisplayBuffer(uint32_t displayId, uint32_t width, uint32_t height);
uint32_t hdLatestScanoutResource(uint32_t scanoutId);
void hdSetScanoutResource(uint32_t scanoutId, uint32_t resourceId);
uint32_t hdScanoutForResource(uint32_t resourceId);
bool hdShouldPresentResource(uint32_t resourceId);

}  // namespace gfxstream
