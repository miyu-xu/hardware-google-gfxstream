/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "EglDispatch.h"

#include <mutex>

#define LOAD_EGL_FUNC(return_type, func_name, signature)                                       \
    do {                                                                                       \
        if (func_name == nullptr) {                                                            \
            func_name =                                                                        \
                reinterpret_cast<return_type(GL_APIENTRY*) signature>(loadSymbol(#func_name)); \
        }                                                                                      \
        if (func_name == nullptr && this->eglGetProcAddress != nullptr) {                      \
            func_name == reinterpret_cast<return_type(GL_APIENTRY*) signature>(                \
                             this->eglGetProcAddress(#func_name));                             \
        }                                                                                      \
        if (func_name == nullptr) {                                                            \
            ERR("Failed to load %s.", #func_name);                                             \
        }                                                                                      \
    } while (0);

#define EGL_DISPATCH_DEFINE_POINTER(return_type, function_name, signature) \
    return_type(*EGLDispatch::function_name) signature = NULL;

LIST_EGL_FUNCTIONS(EGL_DISPATCH_DEFINE_POINTER)

#if defined(ENABLE_DISPATCH_LOG)

// With dispatch debug logging enabled, the original loaded function pointers
// are moved to the "_underlying" suffixed function pointers and then the non
// suffixed functions pointers are updated to be the "_dispatchDebugLogWrapper"
// suffixed functions from the ".impl" files. For example,
//
// EGLint eglGetError_dispatchDebugLogWrapper() {
//   DISPATCH_DEBUG_LOG("eglGetError()");
//   return EGLDispatch::eglGetError_underlying(cap);
// }
//
// EGLDispatch::eglGetError_underlying = dlsym(lib, "eglGetError");
// EGLDispatch::eglGetError = eglGetError_dispatchDebugLogWrapper;

#define translator_egl_const_char const char
#include "OpenGLESDispatch/egl_dispatch_logging_wrappers.impl"

#define LOAD_EGL_FUNC_DEBUG_LOG_WRAPPER(return_type, func_name, signature) \
    do {                                                                   \
        if (func_name != nullptr) {                                        \
            func_name##_underlying = func_name;                            \
            func_name = func_name##_dispatchLoggingWrapper;                \
        }                                                                  \
    } while (0);

#define EGL_DISPATCH_DEFINE_UNDERLYING_POINTER(return_type, function_name, signature) \
    return_type(*EGLDispatch::function_name##_underlying) signature = NULL;

LIST_EGL_FUNCTIONS(EGL_DISPATCH_DEFINE_UNDERLYING_POINTER)

#else

#define LOAD_EGL_FUNC_DEBUG_LOG_WRAPPER(return_type, func_name, signature)

#endif

void EGLDispatch::load(std::function<void*(const char* name)> loadSymbol) {
    static bool sLoaded = false;
    static std::mutex sLoadedMutex;
    std::lock_guard<std::mutex> lock(sLoadedMutex);
    if (sLoaded) return;

    LIST_EGL_FUNCTIONS(LOAD_EGL_FUNC)
    LIST_EGL_FUNCTIONS(LOAD_EGL_FUNC_DEBUG_LOG_WRAPPER)

    sLoaded = true;
}