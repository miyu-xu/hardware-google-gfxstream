// Copyright 2024 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cutils/log.h>
#include <lib/zxio/zxio.h>
#include <limits.h>
#include <services/service_connector.h>
#include <string.h>

#include "os_dirent.h"

// Offsets must match struct os_dirent, static_asserts below.
struct os_dirent_impl {
    ino_t d_ino;
    char d_name[NAME_MAX + 1];
};

static_assert(offsetof(struct os_dirent_impl, d_ino) == offsetof(struct os_dirent, d_ino),
              "d_ino offset mismatch");
static_assert(offsetof(struct os_dirent_impl, d_name) == offsetof(struct os_dirent, d_name),
              "d_ino offset mismatch");

struct os_dir {
    os_dir() {}

    ~os_dir() {
        if (dir_iterator_init) {
            zxio_dirent_iterator_destroy(&iterator);
        }
        if (zxio_init) {
            zxio_close(&io_storage.io, /*should_wait=*/true);
        }
    }

    // Always consumes |dir_channel|
    bool Init(zx_handle_t dir_channel) {
        zx_status_t status = zxio_create(dir_channel, &io_storage);
        if (status != ZX_OK) {
            ALOGE("zxio_create failed: %d", status);
            return false;
        }

        zxio_init = true;

        status = zxio_dirent_iterator_init(&iterator, &io_storage.io);
        if (status != ZX_OK) {
            ALOGE("zxio_dirent_iterator_init failed: %d", status);
            return false;
        }
        dir_iterator_init = true;

        return true;
    }

    struct os_dirent_impl* Next() {
        zxio_dirent_t dirent = {.name = entry.d_name};
        zx_status_t status = zxio_dirent_iterator_next(&iterator, &dirent);
        if (status != ZX_OK) {
            if (status != ZX_ERR_NOT_FOUND) ALOGE("zxio_dirent_iterator_next failed: %d", status);
            return nullptr;
        }

        entry.d_ino = dirent.has.id ? dirent.id : OS_INO_UNKNOWN;
        entry.d_name[dirent.name_length] = '\0';

        return &entry;
    }

   private:
    bool zxio_init = false;
    bool dir_iterator_init = false;
    zxio_storage_t io_storage;
    zxio_dirent_iterator_t iterator;
    struct os_dirent_impl entry;
};

os_dir_t* os_opendir(const char* path) {
    zx_handle_t dir_channel = GetConnectToServiceFunction()(path);
    if (dir_channel == ZX_HANDLE_INVALID) {
        ALOGE("fuchsia_open(%s) failed\n", path);
        return nullptr;
    }

    auto dir = new os_dir();

    if (!dir->Init(dir_channel)) {
        delete dir;
        return nullptr;
    }

    return dir;
}

int os_closedir(os_dir_t* dir) {
    // Assume param is valid.
    delete dir;
    return 0;
}

struct os_dirent* os_readdir(os_dir_t* dir) { return reinterpret_cast<os_dirent*>(dir->Next()); }
