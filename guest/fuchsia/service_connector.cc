// Copyright (C) 2019 The Android Open Source Project
// Copyright (C) 2019 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "services/service_connector.h"

#include <lib/zxio/zxio.h>

namespace {
PFN_ConnectToServiceAddr g_connection_function;
}

void SetConnectToServiceFunction(PFN_ConnectToServiceAddr func) {
  g_connection_function = func;
}

PFN_ConnectToServiceAddr GetConnectToServiceFunction() {
  return g_connection_function;
}

bool IsFuchsiaDeviceAccessible() {
#ifdef VIRTIO_GPU
    const char* path = "/loader-gpu-devices/class/display-coordinator/000";
#else
    const char* path = QEMU_PIPE_PATH;
#endif

  zx_handle_t handle = GetConnectToServiceFunction()(path);
  if (handle == ZX_HANDLE_INVALID)
      return false;

  zxio_storage_t io_storage;
  zx_status_t status = zxio_create(handle, &io_storage);
  if (status != ZX_OK)
      return false;

  status = zxio_close(&io_storage.io, /*should_wait=*/true);
  if (status != ZX_OK)
      return false;

  return true;
}
