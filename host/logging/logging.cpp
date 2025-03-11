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

#include "logging/logging.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <type_traits>

stream_renderer_debug_callback globalDebugCallback = nullptr;
void* loggingUserData = nullptr;

#define MAX_DEBUG_BUFFER_SIZE 512
#define ELLIPSIS "...\0"
#define ELLIPSIS_LEN 4

// Define the typedef for emulogger
typedef void (*emulogger)(char severity, const char* file, unsigned int line, int64_t timestamp_us,
                          const char* message);

// Template to enable the method call if gfxstream_logger_t equals emulogger
template <typename T>
typename std::enable_if<std::is_same<T, emulogger>::value, bool>::type call_logger_if_valid(
    T logger, char severity, const char* file, unsigned int line, int64_t timestamp_us,
    const char* message) {
    // Call the logger and return true if the type matches
    if (!logger) {
        return false;
    }
    logger(severity, file, line, timestamp_us, message);
    return true;
}

// Template for invalid logger types (returns false if types don't match)
template <typename T>
typename std::enable_if<!std::is_same<T, emulogger>::value, bool>::type call_logger_if_valid(
    T, char, const char*, unsigned int, int64_t, const char*) {
    // Return false if the type doesn't match
    return false;
}

static void append_truncation_marker(char* buf, int remaining_size) {
    // Safely append truncation marker "..." if buffer has enough space
    if (remaining_size >= ELLIPSIS_LEN) {
        strncpy(buf + remaining_size - ELLIPSIS_LEN, ELLIPSIS, ELLIPSIS_LEN);
    } else if (remaining_size >= 1) {
        buf[remaining_size - 1] = '\0';
    } else {
        // Oh oh.. In theory this shouldn't happen.
        assert(false);
    }
}

static void log_with_prefix(char*& buf, int& remaining_size, const char* file, int line,
                            const char* pretty_function) {
    // Add logging prefix if necessary
    int formatted_len = snprintf(buf, remaining_size, "[%s(%d)] %s ", file, line, pretty_function);

    // Handle potential truncation
    if (formatted_len >= remaining_size) {
        append_truncation_marker(buf, remaining_size);
        remaining_size = 0;
    } else {
        buf += formatted_len;             // Adjust buf
        remaining_size -= formatted_len;  // Reduce remaining buffer size
    }
}

static char translate_severity(uint32_t type) {
    switch (type) {
        case STREAM_RENDERER_DEBUG_ERROR:
            return 'E';
        case STREAM_RENDERER_DEBUG_WARN:
            return 'W';
        case STREAM_RENDERER_DEBUG_INFO:
            return 'I';
        case STREAM_RENDERER_DEBUG_DEBUG:
            return 'D';
        default:
            return 'D';
    }
}

void stream_renderer_log(uint32_t type, const char* file, int line, const char* pretty_function,
                         const char* format, ...) {
    char printbuf[MAX_DEBUG_BUFFER_SIZE];
    char* buf = printbuf;
    int remaining_size = MAX_DEBUG_BUFFER_SIZE;
    static_assert(MAX_DEBUG_BUFFER_SIZE > 4);

    // Add the logging prefix if needed
#ifdef CONFIG_AEMU
    static gfxstream_logger_t gfx_logger = get_gfx_stream_logger();
    if (!gfx_logger) {
        log_with_prefix(buf, remaining_size, file, line, pretty_function);
    }
#else
    log_with_prefix(buf, remaining_size, file, line, pretty_function);
#endif

    // Format the message with variable arguments
    va_list args;
    va_start(args, format);
    int formatted_len = vsnprintf(buf, remaining_size, format, args);
    va_end(args);

    // Handle potential truncation
    if (formatted_len >= remaining_size) {
        append_truncation_marker(buf, remaining_size);
    }

#ifdef CONFIG_AEMU
    // Forward to emulator?
    if (call_logger_if_valid(gfx_logger, translate_severity(type), file, line, 0, printbuf)) {
        return;
    }
#endif

    // To a gfxstream debugger?
    if (loggingUserData && globalDebugCallback) {
        struct stream_renderer_debug debug = {0};
        debug.debug_type = type;
        debug.message = &printbuf[0];
        globalDebugCallback(loggingUserData, &debug);
    } else {
        // Cannot use logging routines, fallback to stderr
        fprintf(stderr, "stream_renderer_log error: %s\n", printbuf);
    }
}

void stream_renderer_set_debug_callback(void* globalUserData, stream_renderer_debug_callback cb) {
    loggingUserData = globalUserData;
    globalDebugCallback = cb;
}
