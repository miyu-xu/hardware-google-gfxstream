// Copyright (C) 2025 Google Inc.
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

#ifndef GFXSTREAM_LOGGING_H
#define GFXSTREAM_LOGGING_H

#include <cstdarg>
#include <cstdint>

// for stream_renderer_debug_callback
#include "gfxstream/virtio-gpu-gfxstream-renderer.h"

void stream_renderer_set_debug_callback(void* globalUserData, stream_renderer_debug_callback cb);

// Log level of gfxstream
#ifndef STREAM_RENDERER_LOG_LEVEL
#define STREAM_RENDERER_LOG_LEVEL STREAM_RENDERER_DEBUG_INFO
#endif

void stream_renderer_log(uint32_t type, const char* file, int line, const char* pretty_function,
                         const char* format, ...);

#if STREAM_RENDERER_LOG_LEVEL >= STREAM_RENDERER_DEBUG_ERROR
#define stream_renderer_error(format, ...)                                                        \
    do {                                                                                          \
        stream_renderer_log(STREAM_RENDERER_DEBUG_ERROR, __FILE__, __LINE__, __PRETTY_FUNCTION__, \
                            format, ##__VA_ARGS__);                                               \
    } while (0)
#else
#define stream_renderer_error(format, ...)
#endif

#if STREAM_RENDERER_LOG_LEVEL >= STREAM_RENDERER_DEBUG_WARN
#define stream_renderer_warn(format, ...)                                                        \
    do {                                                                                         \
        stream_renderer_log(STREAM_RENDERER_DEBUG_WARN, __FILE__, __LINE__, __PRETTY_FUNCTION__, \
                            format, ##__VA_ARGS__);                                              \
    } while (0)
#else
#define stream_renderer_warn(format, ...)
#endif

#if STREAM_RENDERER_LOG_LEVEL >= STREAM_RENDERER_DEBUG_INFO
#define stream_renderer_info(format, ...)                                                         \
    do {                                                                                          \
        stream_renderer_log(STREAM_RENDERER_DEBUG_INFO, __FILE__, __LINE__, __FUNCTION__, format, \
                            ##__VA_ARGS__);                                                       \
    } while (0)
#else
#define stream_renderer_info(format, ...)
#endif

#if STREAM_RENDERER_LOG_LEVEL >= STREAM_RENDERER_DEBUG_DEBUG
#define stream_renderer_debug(format, ...)                                                        \
    do {                                                                                          \
        stream_renderer_log(STREAM_RENDERER_DEBUG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, \
                            format, ##__VA_ARGS__);                                               \
    } while (0)
#else
#define stream_renderer_debug(format, ...)
#endif

#endif
