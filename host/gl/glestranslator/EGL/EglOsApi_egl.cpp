/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include "EglOsApi.h"
#include "GLcommon/GLLibrary.h"
#include "ShaderCache.h"
#include "aemu/base/SharedLibrary.h"
#include "aemu/base/system/System.h"
#include "host-common/logging.h"
#include "host-common/opengl/misc.h"

#ifdef ANDROID
#include <android/native_window.h>
#endif

#ifdef __linux__
#include "X11Support.h"
#include "X11ErrorHandler.h"
#endif

#ifdef __QNX__
#include "screen/screen.h"
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <cstring>
#include <memory>
#include <vector>

#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__);
#define CHECK_EGL_ERR                                                                 \
    {                                                                                 \
        EGLint err = s_eglDispatch.eglGetError();                                     \
        if (err != EGL_SUCCESS)                                                       \
            D("%s: %s %d get egl error %d\n", __FUNCTION__, __FILE__, __LINE__, err); \
    }
#else
#define D(...) ((void)0);
#define CHECK_EGL_ERR ((void)0);
#endif

#if defined(__WIN32) || defined(_MSC_VER)

static const char* kEGLLibName = "libEGL.dll";
static const char* kGLES2LibName = "libGLESv2.dll";

#elif defined(__linux__)


static const char* kEGLLibName = "libEGL.so";
static const char* kGLES2LibName = "libGLESv2.so";

static const char* kEGLLibNameAlt = "libEGL.so.1";
static const char* kGLES2LibNameAlt = "libGLESv2.so.2";

#elif defined(__QNX__)

static const char* kEGLLibName = "libEGL.so";
static const char* kGLES2LibName = "libGLESv2.so";

#else // __APPLE__

#include "MacNative.h"

static const char* kEGLLibName = "libEGL.dylib";
static const char* kGLES2LibName = "libGLESv2.dylib";

#endif // __APPLE__

namespace {
using namespace EglOS;

EGLDispatch s_eglDispatch;

class EglOsEglLibrary {
   public:
    EglOsEglLibrary() {
        D("loading %s\n", kEGLLibName);
        char error[256];
        mLib = android::base::SharedLibrary::open(kEGLLibName, error, sizeof(error));
        if (!mLib) {
#ifdef __linux__
            ERR("%s: Could not open EGL library %s [%s]. Trying again with [%s]", __FUNCTION__,
                kEGLLibName, error, kEGLLibNameAlt);
            mLib = android::base::SharedLibrary::open(kEGLLibNameAlt, error, sizeof(error));
            if (!mLib) {
                ERR("%s: Could not open EGL library %s [%s]", __FUNCTION__,
                    kEGLLibNameAlt, error);
            }
#else
            ERR("%s: Could not open EGL library %s [%s]", __FUNCTION__,
                kEGLLibName, error);
#endif
        }

        s_eglDispatch.load([&](const char* name) { return (void*)mLib->findSymbol(name); });
    }
    ~EglOsEglLibrary() = default;

   private:
    android::base::SharedLibrary* mLib = nullptr;
};

class EglOsGlLibrary : public GlLibrary {
public:
    EglOsGlLibrary() {
        char error[256];
        mLib = android::base::SharedLibrary::open(kGLES2LibName, error, sizeof(error));
        if (!mLib) {
#ifdef __linux__
            ERR("%s: Could not open GL library %s [%s]. Trying again with [%s]", __FUNCTION__,
                kGLES2LibName, error, kGLES2LibNameAlt);
            mLib = android::base::SharedLibrary::open(kGLES2LibNameAlt, error, sizeof(error));
            if (!mLib) {
                ERR("%s: Could not open GL library %s [%s]", __FUNCTION__,
                    kGLES2LibNameAlt, error);
            }
#else
            ERR("%s: Could not open GL library %s [%s]", __FUNCTION__,
                kGLES2LibName, error);
#endif
        }
    }
    GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        return reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
    }
    ~EglOsGlLibrary() = default;

private:
    android::base::SharedLibrary* mLib = nullptr;
};

class EglOsEglPixelFormat : public EglOS::PixelFormat {
public:
    EglOsEglPixelFormat(EGLConfig configId, EGLint clientCtxVer)
        : mConfigId(configId), mClientCtxVer(clientCtxVer) {}
    PixelFormat* clone() {
        return new EglOsEglPixelFormat(mConfigId, mClientCtxVer);
    }
    EGLConfig mConfigId;
    EGLint mClientCtxVer;
#ifdef __APPLE__
    int mRedSize = 0;
    int mGreenSize = 0;
    int mBlueSize = 0;
#endif // __APPLE__
};

class EglOsEglContext : public EglOS::Context {
public:
 EglOsEglContext(EGLDisplay display, EGLContext context) : mDisplay(display), mNativeCtx(context) {}

 virtual ~EglOsEglContext() {
     D("%s %p\n", __FUNCTION__, mNativeCtx);
     if (!s_eglDispatch->eglDestroyContext(mDisplay, mNativeCtx)) {
         // TODO: print a better error message
     }
    }

    EGLContext context() const {
        return mNativeCtx;
    }

    virtual void* getNative() { return (void*)mNativeCtx; }
private:
 EGLDispatch* s_eglDispatch = nullptr;
 EGLDisplay mDisplay;
 EGLContext mNativeCtx;
};

class EglOsEglSurface : public EglOS::Surface {
public:
    EglOsEglSurface(SurfaceType type,
                    EGLSurface eglSurface,
                    EGLNativeWindowType win = 0)
        : EglOS::Surface(type), mHndl(eglSurface), mWin(win) {}
    EGLSurface getHndl() { return mHndl; }
    EGLNativeWindowType getWin() { return mWin; }

private:
    EGLSurface mHndl;
    EGLNativeWindowType mWin;
};

class EglOsEglDisplay : public EglOS::Display {
public:
    EglOsEglDisplay(bool nullEgl);
    ~EglOsEglDisplay();
    virtual EglOS::GlesVersion getMaxGlesVersion();
    virtual const char* getExtensionString();
    virtual const char* getVendorString();
    virtual EGLImage createImageKHR(
            EGLDisplay dpy,
            EGLContext ctx,
            EGLenum target,
            EGLClientBuffer buffer,
            const EGLint *attrib_list);
    virtual EGLBoolean destroyImageKHR(
            EGLDisplay dpy,
            EGLImage image);
    virtual EGLDisplay getNative();
    void queryConfigs(int renderableType,
                      AddConfigCallback* addConfigFunc,
                      void* addConfigOpaque);
    virtual std::shared_ptr<Context>
    createContext(EGLint profileMask,
                  const PixelFormat* pixelFormat,
                  Context* sharedContext) override;
    Surface* createPbufferSurface(const PixelFormat* pixelFormat,
                                  const PbufferInfo* info);
    Surface* createWindowSurface(PixelFormat* pf, EGLNativeWindowType win);
    bool releasePbuffer(Surface* pb);
    bool makeCurrent(Surface* read, Surface* draw, Context* context);
    EGLBoolean releaseThread();
    void swapBuffers(Surface* srfc);
    bool isValidNativeWin(Surface* win);
    bool isValidNativeWin(EGLNativeWindowType win);
    bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                     const PixelFormat* pixelFormat,
                                     unsigned int* width,
                                     unsigned int* height);
    void* eglGetProcAddress(const char* func) { return s_eglDispatch.eglGetProcAddress(func); }

    EGLint eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback, const EGLAttrib* attribs) {
        if (s_eglDispatch.eglDebugMessageControlKHR == nullptr) {
            return EGL_BAD_ATTRIBUTE;
        }
        return s_eglDispatch.eglDebugMessageControlKHR(callback, attribs);
    }

private:
    bool mVerbose = false;
    EGLDisplay mDisplay = EGL_NO_DISPLAY;
    EglOsEglLibrary mEglLibrary;
    bool mHeadless = false;
    std::string mClientExts;
    std::string mVendor;
    GlesVersion mGlesVersion;

#ifdef __linux__
    ::Display* mGlxDisplay = nullptr;
#endif // __linux__
};

EglOsEglDisplay::EglOsEglDisplay(bool nullEgl) {
    mVerbose = android::base::getEnvironmentVariable("ANDROID_EMUGL_VERBOSE") == "1";

    if (nullEgl) {
#ifdef EGL_ANGLE_platform_angle
        const EGLAttrib attr[] = {
            EGL_PLATFORM_ANGLE_TYPE_ANGLE,
            EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE,
            EGL_NONE
        };

        mDisplay = s_eglDispatch.eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                                                       (void*)EGL_DEFAULT_DISPLAY, attr);

        if (mDisplay == EGL_NO_DISPLAY) {
            fprintf(stderr, "%s: no display found that supports null backend\n", __func__);
        }
#else
        fprintf(stderr, "EGL Null display not compiled, falling back to default display\n");
#endif
    } else if (android::base::getEnvironmentVariable("ANDROID_EMUGL_EXPERIMENTAL_FAST_PATH") == "1") {
#ifdef EGL_ANGLE_platform_angle
        const EGLAttrib attr[] = {
            EGL_PLATFORM_ANGLE_TYPE_ANGLE,
            EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
            EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE,
            EGL_EXPERIMENTAL_PRESENT_PATH_FAST_ANGLE,
            EGL_NONE
        };

        mDisplay = s_eglDispatch.eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                                                       (void*)EGL_DEFAULT_DISPLAY, attr);

        if (mDisplay == EGL_NO_DISPLAY) {
            fprintf(stderr, "%s: no display found that supports the requested extensions\n", __func__);
        }
#endif
    }

    if (mDisplay == EGL_NO_DISPLAY) mDisplay = s_eglDispatch.eglGetDisplay(EGL_DEFAULT_DISPLAY);

    s_eglDispatch.eglInitialize(mDisplay, nullptr, nullptr);
    s_eglDispatch.eglSwapInterval(mDisplay, 0);
    auto clientExts = s_eglDispatch.eglQueryString(mDisplay, EGL_EXTENSIONS);
    auto vendor = s_eglDispatch.eglQueryString(mDisplay, EGL_VENDOR);

    if (mVerbose) {
        fprintf(stderr, "%s: client exts: [%s]\n", __func__, clientExts);
    }

    if (clientExts) {
        mClientExts = clientExts;
    }

    if (vendor) {
        mVendor = vendor;
    }

    s_eglDispatch.eglBindAPI(EGL_OPENGL_ES_API);
    CHECK_EGL_ERR

    mHeadless = android::base::getEnvironmentVariable("ANDROID_EMU_HEADLESS") == "1";

#ifdef ANDROID
    mGlxDisplay = nullptr;
#elif defined(__linux__)
    if (mHeadless) mGlxDisplay = nullptr;
    else mGlxDisplay = getX11Api()->XOpenDisplay(0);
#endif // __linux__

    if (clientExts != nullptr && emugl::hasExtension(clientExts, "EGL_ANDROID_blob_cache")) {
        s_eglDispatch.eglSetBlobCacheFuncsANDROID(mDisplay, SetBlob, GetBlob);
    }

    mGlesVersion = GlesVersion::ES2;

    static const EGLint gles3ConfigAttribs[] =
        { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT, EGL_NONE };

    static const EGLint pbufAttribs[] =
        { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };

    static const EGLint gles31Attribs[] =
       { EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
         EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE };

    static const EGLint gles30Attribs[] =
       { EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
         EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE };

    EGLConfig config;

    int numConfigs;
    if (s_eglDispatch.eglChooseConfig(mDisplay, gles3ConfigAttribs, &config, 1, &numConfigs) &&
        numConfigs != 0) {
        EGLSurface surface = s_eglDispatch.eglCreatePbufferSurface(mDisplay, config, pbufAttribs);
        if (surface != EGL_NO_SURFACE) {
            EGLContext ctx =
                s_eglDispatch.eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, gles31Attribs);

            if (ctx != EGL_NO_CONTEXT) {
                mGlesVersion = GlesVersion::ES31;
            } else {
                ctx =
                    s_eglDispatch.eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, gles30Attribs);
                if (ctx != EGL_NO_CONTEXT) {
                    mGlesVersion = GlesVersion::ES30;
                }
            }
            s_eglDispatch.eglDestroySurface(mDisplay, surface);
            if (ctx != EGL_NO_CONTEXT) {
                s_eglDispatch.eglDestroyContext(mDisplay, ctx);
            }
        }
    }
};

EglOsEglDisplay::~EglOsEglDisplay() {
#ifdef ANDROID
#elif defined(__linux__)
    if (mGlxDisplay) getX11Api()->XCloseDisplay(mGlxDisplay);
#endif // __linux__
}

EglOS::GlesVersion EglOsEglDisplay::getMaxGlesVersion() {
    // Maximum GLES3.1
    // GLES3.2 will also need some more autogen + enums if anyone is interested.
    return mGlesVersion;
}

const char* EglOsEglDisplay::getExtensionString() {
    return mClientExts.c_str();
}

const char* EglOsEglDisplay::getVendorString() {
    return mVendor.c_str();
}

EGLImage EglOsEglDisplay::createImageKHR(
        EGLDisplay dpy,
        EGLContext ctx,
        EGLenum target,
        EGLClientBuffer buffer,
        const EGLint *attrib_list) {
    if (s_eglDispatch.eglCreateImageKHR) {
        return s_eglDispatch.eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
    } else {
        return EGL_NO_IMAGE_KHR;
    }
}

EGLBoolean EglOsEglDisplay::destroyImageKHR(
        EGLDisplay dpy,
        EGLImage image) {
    if (s_eglDispatch.eglDestroyImage) {
        return s_eglDispatch.eglDestroyImageKHR(dpy, image);
    } else {
        return EGL_FALSE;
    }
}

EGLDisplay EglOsEglDisplay::getNative() {
    return mDisplay;
}

void EglOsEglDisplay::queryConfigs(int renderableType,
                                   AddConfigCallback* addConfigFunc,
                                   void* addConfigOpaque) {
    D("%s\n", __FUNCTION__);
    // ANGLE does not support GLES1 uses core profile engine.
    // Querying underlying EGL with a conservative set of bits.
    renderableType &= ~EGL_OPENGL_ES_BIT;

    const EGLint framebuffer_config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_NONE,
    };

    EGLint numConfigs = 0;
    s_eglDispatch.eglChooseConfig(mDisplay, framebuffer_config_attributes, nullptr, 0, &numConfigs);
    CHECK_EGL_ERR
    std::unique_ptr<EGLConfig[]> configs(new EGLConfig[numConfigs]);
    s_eglDispatch.eglChooseConfig(mDisplay, framebuffer_config_attributes, configs.get(),
                                  numConfigs, &numConfigs);
    CHECK_EGL_ERR

    if (mVerbose) {
        fprintf(stderr, "%s: num configs: %d\n", __func__, numConfigs);
    }

    for (int i = 0; i < numConfigs; i++) {
        const EGLConfig cfg = configs.get()[i];
        ConfigInfo configInfo;
        // We do not have recordable_android
        configInfo.recordable_android = 0;
        EGLint _renderableType;
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_RENDERABLE_TYPE, &_renderableType);
        // We do emulate GLES1
        configInfo.renderable_type = _renderableType | EGL_OPENGL_ES_BIT;

        configInfo.frmt = new EglOsEglPixelFormat(cfg, _renderableType);
        D("config %p renderable type 0x%x\n", cfg, _renderableType);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_RED_SIZE, &configInfo.red_size);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_GREEN_SIZE, &configInfo.green_size);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_BLUE_SIZE, &configInfo.blue_size);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_ALPHA_SIZE, &configInfo.alpha_size);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_CONFIG_CAVEAT,
                                         (EGLint*)&configInfo.caveat);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_DEPTH_SIZE, &configInfo.depth_size);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_LEVEL, &configInfo.frame_buffer_level);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_WIDTH,
                                         &configInfo.max_pbuffer_width);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_HEIGHT,
                                         &configInfo.max_pbuffer_height);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_PIXELS,
                                         &configInfo.max_pbuffer_size);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_RENDERABLE,
                                         (EGLint*)&configInfo.native_renderable);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_ID,
                                         &configInfo.native_visual_id);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_TYPE,
                                         &configInfo.native_visual_type);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_SAMPLES, &configInfo.samples_per_pixel);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_STENCIL_SIZE, &configInfo.stencil_size);

        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_SURFACE_TYPE, &configInfo.surface_type);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_TYPE,
                                         (EGLint*)&configInfo.transparent_type);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_RED_VALUE,
                                         &configInfo.trans_red_val);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_GREEN_VALUE,
                                         &configInfo.trans_green_val);
        s_eglDispatch.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_BLUE_VALUE,
                                         &configInfo.trans_blue_val);
        CHECK_EGL_ERR
#ifdef __APPLE__
        ((EglOsEglPixelFormat*)configInfo.frmt)->mRedSize = configInfo.red_size;
        ((EglOsEglPixelFormat*)configInfo.frmt)->mGreenSize = configInfo.green_size;
        ((EglOsEglPixelFormat*)configInfo.frmt)->mBlueSize = configInfo.blue_size;
#endif // __APPLE__
        addConfigFunc(addConfigOpaque, &configInfo);
    }
    D("Host gets %d configs\n", numConfigs);
}

std::shared_ptr<Context>
EglOsEglDisplay::createContext(EGLint profileMask,
                               const PixelFormat* pixelFormat,
                               Context* sharedContext) {
    (void)profileMask;

    D("%s\n", __FUNCTION__);
    const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    D("with config %p\n", format->mConfigId);

    // Always GLES3
    std::vector<EGLint> attributes = { EGL_CONTEXT_CLIENT_VERSION, 3 };
    auto exts = s_eglDispatch.eglQueryString(mDisplay, EGL_EXTENSIONS);
    auto vendor = s_eglDispatch.eglQueryString(mDisplay, EGL_VENDOR);

    // TODO (b/207426737): remove Imagination-specific workaround
    bool disable_robustness = vendor && (strcmp(vendor, "Imagination Technologies") == 0);

    bool disableValidation = android::base::getEnvironmentVariable("ANDROID_EMUGL_EGL_VALIDATION") == "0";
    if (exts != nullptr && emugl::hasExtension(exts, "EGL_KHR_create_context_no_error") && disableValidation) {
        attributes.push_back(EGL_CONTEXT_OPENGL_NO_ERROR_KHR);
        attributes.push_back(EGL_TRUE);
    }

    if (exts != nullptr && emugl::hasExtension(exts, "EGL_EXT_create_context_robustness") && !disable_robustness) {
        attributes.push_back(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT);
        attributes.push_back(EGL_LOSE_CONTEXT_ON_RESET_EXT);
    }
    attributes.push_back(EGL_NONE);

    // TODO: support GLES3.1
    EglOsEglContext* nativeSharedCtx = (EglOsEglContext*)sharedContext;
    EGLContext newNativeCtx = s_eglDispatch.eglCreateContext(
        mDisplay, format->mConfigId, nativeSharedCtx ? nativeSharedCtx->context() : nullptr,
        attributes.data());
    CHECK_EGL_ERR

    std::shared_ptr<Context> res = std::make_shared<EglOsEglContext>(mDisplay, newNativeCtx);
    D("%s done\n", __FUNCTION__);
    return res;
}

Surface* EglOsEglDisplay::createPbufferSurface(const PixelFormat* pixelFormat,
                                               const PbufferInfo* info) {
    // D("%s\n", __FUNCTION__);
    // const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    // EGLint attrib[] = {EGL_WIDTH,
    //                    info->width,
    //                    EGL_HEIGHT,
    //                    info->height,
    //                    EGL_LARGEST_PBUFFER,
    //                    info->largest,
    //                    EGL_TEXTURE_FORMAT,
    //                    info->format,
    //                    EGL_TEXTURE_TARGET,
    //                    info->target,
    //                    EGL_MIPMAP_TEXTURE,
    //                    info->hasMipmap,
    //                    EGL_NONE};
    // EGLSurface surface = s_eglDispatch.eglCreatePbufferSurface(
    //         mDisplay, format->mConfigId, attrib);
    // CHECK_EGL_ERR
    // if (surface == EGL_NO_SURFACE) {
    //     D("create pbuffer surface failed\n");
    //     return nullptr;
    // }
    // return new EglOsEglSurface(EglOS::Surface::PBUFFER, surface);
    return new EglOsEglSurface(EglOS::Surface::PBUFFER, 0);
}

Surface* EglOsEglDisplay::createWindowSurface(PixelFormat* pf,
                                              EGLNativeWindowType win) {
    D("%s\n", __FUNCTION__);
    std::vector<EGLint> surface_attribs;
    auto exts = s_eglDispatch.eglQueryString(mDisplay, EGL_EXTENSIONS);
    if (exts != nullptr && emugl::hasExtension(exts, "EGL_ANGLE_direct_composition")) {
#ifdef EGL_ANGLE_direct_composition
        surface_attribs.push_back(EGL_DIRECT_COMPOSITION_ANGLE);
        surface_attribs.push_back(EGL_TRUE);
#endif
    }
    surface_attribs.push_back(EGL_NONE);
#ifdef __APPLE__
    win = nsGetLayer(win);
#endif
    EGLSurface surface = s_eglDispatch.eglCreateWindowSurface(
        mDisplay, ((EglOsEglPixelFormat*)pf)->mConfigId, win, surface_attribs.data());
    CHECK_EGL_ERR
    if (surface == EGL_NO_SURFACE) {
        D("create window surface failed\n");
        return nullptr;
    }
    return new EglOsEglSurface(EglOS::Surface::WINDOW, surface, win);
}

bool EglOsEglDisplay::releasePbuffer(Surface* pb) {
    D("%s\n", __FUNCTION__);
    if (!pb)
        return false;
    EglOsEglSurface* surface = (EglOsEglSurface*)pb;

    if (!surface->getHndl()) {
        delete surface;
        return true;
    }

    bool ret = s_eglDispatch.eglDestroySurface(mDisplay, surface->getHndl());
    CHECK_EGL_ERR
    D("%s done\n", __FUNCTION__);
    delete surface;
    return ret;
}

bool EglOsEglDisplay::makeCurrent(Surface* read,
                                  Surface* draw,
                                  Context* context) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* readSfc = (EglOsEglSurface*)read;
    EglOsEglSurface* drawSfc = (EglOsEglSurface*)draw;
    EglOsEglContext* ctx = (EglOsEglContext*)context;
    if (ctx && !readSfc) {
        D("warning: makeCurrent a context without surface\n");
        return false;
    }
    D("%s %p\n", __FUNCTION__, ctx ? ctx->context() : nullptr);
    bool ret = s_eglDispatch.eglMakeCurrent(mDisplay, drawSfc ? drawSfc->getHndl() : EGL_NO_SURFACE,
                                            readSfc ? readSfc->getHndl() : EGL_NO_SURFACE,
                                            ctx ? ctx->context() : EGL_NO_CONTEXT);
    if (readSfc) {
        D("make current surface type %d %d\n", readSfc->type(),
          drawSfc->type());
    }
    D("make current %d\n", ret);
    CHECK_EGL_ERR
    return ret;
}

void EglOsEglDisplay::swapBuffers(Surface* surface) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* sfc = (EglOsEglSurface*)surface;
    s_eglDispatch.eglSwapBuffers(mDisplay, sfc->getHndl());
}

EGLBoolean EglOsEglDisplay::releaseThread() {
    D("%s\n", __FUNCTION__);
    return s_eglDispatch.eglReleaseThread();
}

bool EglOsEglDisplay::isValidNativeWin(Surface* win) {
    if (!win) {
        return false;
    }
    EglOsEglSurface* surface = (EglOsEglSurface*)win;
    return surface->type() == EglOsEglSurface::WINDOW &&
           isValidNativeWin(surface->getWin());
}

bool EglOsEglDisplay::isValidNativeWin(EGLNativeWindowType win) {
#ifdef _WIN32
    return IsWindow(win);
#elif defined(ANDROID)
    return true;
#elif defined(__linux__)
    Window root;
    int t;
    unsigned int u;
    X11ErrorHandler handler(mGlxDisplay);
    return getX11Api()->XGetGeometry(mGlxDisplay, win, &root, &t, &t, &u, &u, &u, &u) != 0;
#elif defined(__QNX__)
    int size[2];
    return screen_get_window_property_iv(win, SCREEN_PROPERTY_SIZE, size) != -1;
#else // __APPLE__
    unsigned int width, height;
    return nsGetWinDims(win, &width, &height);
#endif // __APPLE__
}

bool EglOsEglDisplay::checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                 const PixelFormat* pixelFormat,
                                 unsigned int* width,
                                 unsigned int* height) {
#ifdef _WIN32
    RECT r;
    if (!GetClientRect(win, &r)) {
        return false;
    }
    *width = r.right - r.left;
    *height = r.bottom - r.top;
    return true;
#elif defined(ANDROID)
    *width = ANativeWindow_getWidth((ANativeWindow*)win);
    *height = ANativeWindow_getHeight((ANativeWindow*)win);
    return true;
#elif defined(__linux__)
    //TODO: to check what does ATI & NVIDIA enforce on win pixelformat
    unsigned int depth, border;
    int x, y;
    Window root;
    X11ErrorHandler handler(mGlxDisplay);
    return getX11Api()->XGetGeometry(
            mGlxDisplay, win, &root, &x, &y, width, height, &border, &depth);
#elif defined(__QNX__)
    int size[2];
    if (screen_get_window_property_iv(win, SCREEN_PROPERTY_SIZE, size) == -1) {
        return false;
    }
    *width = size[0];
    *height = size[1];
    return true;
#else // __APPLE__
    bool ret = nsGetWinDims(win, width, height);

    const EglOsEglPixelFormat* format = (EglOsEglPixelFormat*)pixelFormat;
    int r = format->mRedSize;
    int g = format->mGreenSize;
    int b = format->mBlueSize;

    bool match = nsCheckColor(win, r + g + b);

    return ret && match;
#endif // __APPLE__
}

static EglOsEglDisplay* sHostDisplay(bool nullEgl = false) {
    static EglOsEglDisplay* d = new EglOsEglDisplay(nullEgl);
    return d;
}

class EglEngine : public EglOS::Engine {
public:
 EglEngine(bool nullEgl)
     :
#ifdef __QNX__  // Ensure libEGL is loaded prior to libGLES
       mDisplay(sHostDisplay(nullEgl)),
#endif
       EglOS::Engine(),
       mUseNullEgl(nullEgl) {
 }

 ~EglEngine() = default;

 EglOS::Display* getDefaultDisplay() {
     D("%s\n", __FUNCTION__);
     return sHostDisplay(mUseNullEgl);
 }
 GlLibrary* getGlLibrary() {
     D("%s\n", __FUNCTION__);
     return &mGlLib;
 }
 void* eglGetProcAddress(const char* func) { return sHostDisplay()->eglGetProcAddress(func); }
 virtual EglOS::Surface* createWindowSurface(PixelFormat* pf, EGLNativeWindowType wnd) {
     D("%s\n", __FUNCTION__);
     return sHostDisplay()->createWindowSurface(pf, wnd);
 }

 EGLint eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback, const EGLAttrib* attribs) override {
     return sHostDisplay()->eglDebugMessageControlKHR(callback, attribs);
 }

private:
#ifdef __QNX__
 EglOsEglDisplay* mDisplay;
#endif
 EglOsGlLibrary mGlLib;
 bool mUseNullEgl;
};

}  // namespace

static EglEngine* sHostEngine(bool nullEgl) {
    static EglEngine* res = new EglEngine(nullEgl);
    return res;
}

namespace EglOS {
Engine* getEgl2EglHostInstance(bool nullEgl) {
    D("%s\n", __FUNCTION__);
    return sHostEngine(nullEgl);
}
}  // namespace EglOS
