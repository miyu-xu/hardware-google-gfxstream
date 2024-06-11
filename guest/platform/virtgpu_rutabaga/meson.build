# Copyright 2022 Android Open Source Project
# SPDX-License-Identifier: MIT

files_lib_platform = files(
  '../VirtGpu.cpp',
  'VirtGpuRutabagaDevice.cpp',
  'VirtGpuRutabagaBlobMapping.cpp',
  'VirtGpuRutabagaBlob.cpp',
  'VirtGpuRutabagaSync.cpp',
)

lib_platform = static_library(
   'platform',
   files_lib_platform,
   cpp_args: gfxstream_guest_args,
   include_directories: [inc_platform, inc_android_compat],
   dependencies: virtgpu_rutabaga_dep,
)
