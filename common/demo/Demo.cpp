#include <iostream>
#include <thread>

#include "Egl.h"
#include "Expected.h"
#include "Gles.h"
#include "Latch.h"

namespace gfxstream {

static void EGLAPIENTRY EglDebugCallback(EGLenum error,
                                         const char *command,
                                         EGLint messageType,
                                         EGLLabelKHR threadLabel,
                                         EGLLabelKHR objectLabel,
                                         const char *message) {
    std::cout << "EglDebugCallback "
              << " error:"
              << error
              << " command:"
              << command
              << " threadLabel:"
              << threadLabel
              << " objectLabel:"
              << objectLabel
              << " message:"
              << message
              << std::endl;
}

static void GL_APIENTRY GlDebugCallback(GLenum source,
                                        GLenum type,
                                        GLuint id,
                                        GLenum severity,
                                        GLsizei length,
                                        const GLchar *message,
                                        const void *userParam) {
    std::cout << "EglDebugCallback "
              << " source:"
              << source
              << " type:"
              << type
              << " id:"
              << id
              << " severity:"
              << severity
              << " message:"
              << message
              << std::endl;
}

void PrintEglErrors(Egl& egl) {
    EGLint error = egl.eglGetError();
    if (error != EGL_SUCCESS) {
        std::cout << "EGL error: " << error << std::endl;
    }
}

#define EXPECT_NO_GL_ERRORS()                                                       \
    do {                                                                            \
        if (GLenum error = gles.glGetError(); error != GL_NO_ERROR) {               \
            return gfxstream::unexpected(std::string("Encountered GL error ") +     \
                                         std::to_string(error) +                    \
                                         std::string(" at line ") +                 \
                                         std::to_string(__LINE__));                 \
        }                                                                           \
    } while (0);

gfxstream::expected<Ok, std::string> Demo() {
    auto egl = GFXSTREAM_EXPECT(Egl::Load());

    EGLDisplay display = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        return gfxstream::unexpected("Failed to find display.");
    }

    EGLint client_version_major = 0;
    EGLint client_version_minor = 0;
    if (egl.eglInitialize(display, &client_version_major, &client_version_minor) != EGL_TRUE) {
        return gfxstream::unexpected("Failed to initialize display.");
    }
    std::cout << "EGL client version major: " << client_version_major << std::endl;
    std::cout << "EGL client version minor: " << client_version_minor << std::endl;

    const std::string version_string = egl.eglQueryString(display, EGL_VERSION);
    if (version_string.empty()) {
        return gfxstream::unexpected("Failed to query client version.");
    }
    std::cout << "EGL_VERSION: " << version_string << std::endl;

    const std::string vendor_string = egl.eglQueryString(display, EGL_VENDOR);
    if (vendor_string.empty()) {
        return gfxstream::unexpected("Failed to query vendor.");
    }
    std::cout << "EGL_VENDOR: " << vendor_string << std::endl;

    const std::string extensions_string = egl.eglQueryString(display, EGL_EXTENSIONS);
    if (extensions_string.empty()) {
        return gfxstream::unexpected("Failed to query extensions.");
    }
    std::cout << "EGL_EXTENSIONS: " << extensions_string << std::endl;

    const std::string display_apis_string = egl.eglQueryString(display, EGL_CLIENT_APIS);
    if (display_apis_string.empty()) {
        return gfxstream::unexpected("Failed to query display apis.");
    }
    std::cout << "EGL_CLIENT_APIS: " << display_apis_string << std::endl;

    if (egl.eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        return gfxstream::unexpected("Failed to bind GLES API.");
    }

    const EGLAttrib controls[] = {
            EGL_DEBUG_MSG_CRITICAL_KHR,
            EGL_TRUE,
            EGL_DEBUG_MSG_ERROR_KHR,
            EGL_TRUE,
            EGL_DEBUG_MSG_WARN_KHR,
            EGL_TRUE,
            EGL_DEBUG_MSG_INFO_KHR,
            EGL_FALSE,
            EGL_NONE,
            EGL_NONE,
        };

    if (egl.eglDebugMessageControlKHR(&EglDebugCallback, controls) == EGL_SUCCESS) {
        std::cout << "Set debug message control callback." << std::endl;
    } else {
        std::cout << "Failed to set debug message control callback." << std::endl;
    }

    const GLint configAttribs[] = {
        EGL_RED_SIZE,        8,                   //
        EGL_GREEN_SIZE,      8,                   //
        EGL_BLUE_SIZE,       8,                   //
        EGL_ALPHA_SIZE,      8,                   //
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,  //
        EGL_NONE,                                 //
    };
    EGLint num_configs = 0;
    egl.eglChooseConfig(display, configAttribs, nullptr, 0, &num_configs);
    if (num_configs == 0) {
        return gfxstream::unexpected("Failed to find matching config.");
    }

    std::vector<EGLConfig> configs(num_configs);
    egl.eglChooseConfig(display, configAttribs, configs.data(), num_configs, &num_configs);
    if (num_configs == 0) {
        return gfxstream::unexpected("Failed to find matching config.");
    }

    EGLConfig config = configs[0];
    std::cout << "EGL config " << config << std::endl;

    const GLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,  //
        EGL_NONE,                       //
    };
    EGLContext context = egl.eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        PrintEglErrors(egl);
        return gfxstream::unexpected("Failed to create context.");
    }

    if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) != EGL_TRUE) {
        PrintEglErrors(egl);
        return gfxstream::unexpected("Failed to make context current.");
    }

    auto gles = GFXSTREAM_EXPECT(Gles::LoadFromEgl(&egl));

    const GLubyte* gles_vendor = gles.glGetString(GL_VENDOR);
    if (gles_vendor == nullptr) {
        return gfxstream::unexpected("Failed to query vendor.");
    }
    const std::string gles_vendor_string((const char*)gles_vendor);
    std::cout << "GL_VENDOR: " << gles_vendor_string << std::endl;

    const GLubyte* gles_version = gles.glGetString(GL_VERSION);
    if (gles_version == nullptr) {
        gfxstream::unexpected("Failed to query vendor.");
    }
    const std::string gles_version_string((const char*)gles_version);
    std::cout << "GL_VERSION: " << gles_version_string << std::endl;

    const GLubyte* gles_renderer = gles.glGetString(GL_RENDERER);
    if (gles_renderer == nullptr) {
        gfxstream::unexpected("Failed to query renderer.");
    }
    const std::string gles_renderer_string((const char*)gles_renderer);
    std::cout << "GL_RENDERER: " << gles_renderer_string << std::endl;

    const GLubyte* gles_extensions = gles.glGetString(GL_EXTENSIONS);
    if (gles_extensions == nullptr) {
        return gfxstream::unexpected("Failed to query extensions.");
    }
    const std::string gles_extensions_string((const char*)gles_extensions);
    std::cout << "GL_EXTENSIONS: " << gles_extensions_string << std::endl;

    constexpr const uint32_t width = 1280;
    constexpr const uint32_t height = 720;

    GLuint colorbuffer = 0;
    gles.glGenTextures(1, &colorbuffer);
    gles.glBindTexture(GL_TEXTURE_2D, colorbuffer);
    gles.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXPECT_NO_GL_ERRORS();
    gles.glBindTexture(GL_TEXTURE_2D, 0);

    if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
        PrintEglErrors(egl);
        return gfxstream::unexpected("Failed to make context current.");
    }

    std::vector<std::uint8_t> pixels_1(width * height * 4, 128);
    std::vector<std::uint8_t> pixels_2(width * height * 4, 175);

    EGLContext main_context = context;
    std::mutex main_context_mutex;

    static constexpr const int kNumIterations = 10000;
    for (int iteration = 0; iteration < kNumIterations; iteration++) {

        SimpleLatch threads_initialized{2};

        std::thread virtio_gpu_thread([&]() {

            threads_initialized.count_down();
            threads_initialized.wait();

            {
                // Pretend to do an upload/download:
                std::lock_guard<std::mutex> main_context_lock(main_context_mutex);

                if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, main_context) != EGL_TRUE) {
                    PrintEglErrors(egl);
                    return;
                }

                const auto* pixels = (iteration % 2 == 0) ? pixels_1.data() : pixels_2.data();
                gles.glBindTexture(GL_TEXTURE_2D, colorbuffer);
                gles.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                gles.glBindTexture(GL_TEXTURE_2D, 0);

                if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
                    PrintEglErrors(egl);
                    return;
                }
            }
        });

        std::thread renderthread_thread([&](){
            // RenderThread startup

            EGLContext renderthread_context = egl.eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
            if (renderthread_context == EGL_NO_CONTEXT) {
                PrintEglErrors(egl);
                return;
            }

            if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderthread_context) != EGL_TRUE) {
                PrintEglErrors(egl);
                return;
            }

            if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
                PrintEglErrors(egl);
                return;
            }

            threads_initialized.count_down();
            threads_initialized.wait();

            // RenderThread shutdown

            {
                std::lock_guard<std::mutex> main_context_lock(main_context_mutex);

                if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, main_context) != EGL_TRUE) {
                    PrintEglErrors(egl);
                    return;
                }

                gles.glFlush();

                if (egl.eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
                    PrintEglErrors(egl);
                    return;
                }
            }

            {
                egl.eglDestroyContext(display, renderthread_context);
                PrintEglErrors(egl);

                egl.eglReleaseThread();
                PrintEglErrors(egl);
            }
        });


        renderthread_thread.join();
        virtio_gpu_thread.join();
    }

    return Ok{};
}

}  // namespace gfxstream

int main() {
    auto ret = gfxstream::Demo();
    if (!ret.ok()) {
        std::cout << std::endl << "Demo failed with: " << ret.error() << std::endl;
    } else {
        std::cout << std::endl << "Demo finished successfully." << std::endl;
    }
}