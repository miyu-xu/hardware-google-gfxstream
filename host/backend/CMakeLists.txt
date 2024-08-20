if (NOT TARGET gfxstream-gl-host-common)
    add_library(
        gfxstream_backend.headers
        INTERFACE)
    target_include_directories(
        gfxstream_backend.headers
        INTERFACE
        include)

    add_library(
        gfxstream_backend)
    target_link_libraries(
        gfxstream_backend
        PUBLIC
        gfxstream_backend.headers)
endif()
