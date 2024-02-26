// Copyright 2024 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MAGMA_VIRTIO_GPU_DEFS_H
#define MAGMA_VIRTIO_GPU_DEFS_H

#include <lib/magma/magma_common_defs.h>

#define MAGMA_VENDOR_VERSION_VIRTIO 1

enum MagmaVirtioGpuQuery {
  // Returns 3d features (simple result)
  kMagmaVirtioGpuQuery3dFeatures = MAGMA_QUERY_VENDOR_PARAM_0,
  kMagmaVirtioGpuQueryCapsetFix = MAGMA_QUERY_VENDOR_PARAM_0 + 1,
  kMagmaVirtioGpuQueryResourceBlob = MAGMA_QUERY_VENDOR_PARAM_0 + 2,
  kMagmaVirtioGpuQueryHostVisible = MAGMA_QUERY_VENDOR_PARAM_0 + 3,
  kMagmaVirtioGpuQueryCrossDevice = MAGMA_QUERY_VENDOR_PARAM_0 + 4,
  kMagmaVirtioGpuQueryContextInit = MAGMA_QUERY_VENDOR_PARAM_0 + 5,
  kMagmaVirtioGpuQuerySupportedCapsetIds = MAGMA_QUERY_VENDOR_PARAM_0 + 6,
  kMagmaVirtioGpuQueryExplicitDebugName = MAGMA_QUERY_VENDOR_PARAM_0 + 7,
  kMagmaVirtioGpuQueryCreateGuestHandle = MAGMA_QUERY_VENDOR_PARAM_0 + 8,
  // Bits 32..47 indicate the capset id, bits 48..63 indicate the version (buffer result).
  kMagmaVirtioGpuQueryCapset = MAGMA_QUERY_VENDOR_PARAM_0 + 10000,
};

#endif  // MAGMA_VIRTIO_GPU_DEFS_H
