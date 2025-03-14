if(GFXSTREAM_ENABLE_HOST_TRACING)
    if (NOT TARGET gfxstream_host_tracing)
        add_library(
            gfxstream_host_tracing.headers
            INTERFACE)
        target_include_directories(
            gfxstream_host_tracing.headers
            INTERFACE
            include)

        add_library(
            gfxstream_host_tracing
            Tracing.cpp)
        target_link_libraries(
            gfxstream_host_tracing
            PUBLIC
            gfxstream_host_tracing.headers
            perfetto)
    endif()
endif()