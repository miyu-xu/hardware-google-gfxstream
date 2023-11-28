add_library(
    gfxstream_opengl_headers
    INTERFACE)
target_include_directories(
    gfxstream_opengl_headers
    INTERFACE
    include)
android_license(TARGET gfxstream_opengl_headers LICENSE Apache-2.0)