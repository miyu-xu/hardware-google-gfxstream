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

#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <functional>

#include "host-common/logging.h"

using EGLCharPtr = const char*;
using EglVoidPtr = void*;
#define LIST_EGL_FUNCTIONS(X)                                                                      \
    X(EglVoidPtr, eglGetProcAddress, (const char* procname))                                       \
    X(EGLBoolean, eglBindAPI, (EGLenum api))                                                       \
    X(EGLBoolean, eglChooseConfig,                                                                 \
      (EGLDisplay display, EGLint const* attrib_list, EGLConfig* configs, EGLint config_size,      \
       EGLint* num_config))                                                                        \
    X(EGLBoolean, eglDestroyContext, (EGLDisplay display, EGLContext context))                     \
    X(EGLBoolean, eglDestroyImage, (EGLDisplay dpy, EGLImage image))                               \
    X(EGLBoolean, eglDestroyImageKHR, (EGLDisplay dpy, EGLImage image))                            \
    X(EGLBoolean, eglDestroySurface, (EGLDisplay display, EGLSurface surface))                     \
    X(EGLBoolean, eglGetConfigAttrib,                                                              \
      (EGLDisplay display, EGLConfig config, EGLint attribute, EGLint * value))                    \
    X(EGLBoolean, eglGetConfigs,                                                                   \
      (EGLDisplay display, EGLConfig * configs, EGLint config_size, EGLint * num_config))          \
    X(EGLBoolean, eglInitialize, (EGLDisplay display, EGLint * major, EGLint * minor))             \
    X(EGLBoolean, eglMakeCurrent,                                                                  \
      (EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context))                  \
    X(EGLBoolean, eglQuerySurface,                                                                 \
      (EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint * value))                  \
    X(EGLBoolean, eglReleaseThread, (void))                                                        \
    X(EGLBoolean, eglSwapBuffers, (EGLDisplay display, EGLSurface surface))                        \
    X(EGLBoolean, eglSwapInterval, (EGLDisplay display, EGLint interval))                          \
    X(EGLBoolean, eglTerminate, (EGLDisplay display))                                              \
    X(EGLCharPtr, eglQueryString, (EGLDisplay dpy, EGLint id))                                     \
    X(EGLContext, eglCreateContext,                                                                \
      (EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const* attrib_list)) \
    X(EGLContext, eglGetCurrentContext, (void))                                                    \
    X(EGLDisplay, eglGetDisplay, (NativeDisplayType native_display))                               \
    X(EGLDisplay, eglGetPlatformDisplay,                                                           \
      (EGLenum platform, void* native_display, const EGLAttrib* attrib_list))                      \
    X(EGLDisplay, eglGetPlatformDisplayEXT,                                                        \
      (EGLenum platform, void* native_display, const EGLint* attrib_list))                         \
    X(EGLenum, eglQueryAPI, (void))                                                                \
    X(EGLImage, eglCreateImage,                                                                    \
      (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer,                     \
       const EGLAttrib* attrib_list))                                                              \
    X(EGLImage, eglCreateImageKHR,                                                                 \
      (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer,                     \
       const EGLint* attrib_list))                                                                 \
    X(EGLint, eglDebugMessageControlKHR, (EGLDEBUGPROCKHR callback, const EGLAttrib* attrib_list)) \
    X(EGLint, eglGetError, (void))                                                                 \
    X(EGLSurface, eglCreatePbufferSurface,                                                         \
      (EGLDisplay display, EGLConfig config, EGLint const* attrib_list))                           \
    X(EGLSurface, eglCreateWindowSurface,                                                          \
      (EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window,                    \
       EGLint const* attrib_list))                                                                 \
    X(EGLSurface, eglGetCurrentSurface, (EGLint readdraw))                                         \
    X(void, eglSetBlobCacheFuncsANDROID,                                                           \
      (EGLDisplay display, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get))

class EGLDispatch {
   public:
#define EGL_DECLARE_METHOD(return_type, function_name, signature) \
    static return_type(*function_name) signature;

    LIST_EGL_FUNCTIONS(EGL_DECLARE_METHOD)

#if defined(ENABLE_DISPATCH_LOG)
#define EGL_DECLARE_UNDERLYING_METHOD(return_type, function_name, signature) \
    static return_type(*function_name##_underlying) signature;

    LIST_EGL_FUNCTIONS(EGL_DECLARE_UNDERLYING_METHOD)
#endif

    void load(std::function<void*(const char*)> loadSymbol);
};
