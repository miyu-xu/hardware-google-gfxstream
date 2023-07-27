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

#include <cutils/log.h>

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
    const char* path = "/loader-gpu-devices/class/virtio-gpu";
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
  
  zxio_dirent_iterator_t iterator;
  status = zxio_dirent_iterator_init(&iterator, &io_storage.io);
  if (status != ZX_OK) {
    ALOGE("zxio_dirent_iterator_init failed: %d", status);
    return false;
  }
 
  while (true) {
    char name[ZXIO_MAX_FILENAME + 1];
    zxio_dirent_t dirent = {.name = name};
    status = zxio_dirent_iterator_next(&iterator, &dirent);
    if (status != ZX_OK) {
        if (status != ZX_ERR_NOT_FOUND) {
          ALOGE("zxio_dirent_iterator_next failed: %d", status);
        }
        return false;
    }

    name[dirent.name_length] = '\0';
    ALOGE("*** got name %s\n", name);
    if (name[0] != '.')
      break;
  }

  status = zxio_close(&io_storage.io, /*should_wait=*/true);
  if (status != ZX_OK)
      return false;

  return true;
}
