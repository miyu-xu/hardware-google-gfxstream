add_library(
    gfxtream-host-backend-logging
    logging.cpp
)

target_include_directories(
    gfxstream-host-backend-logging
    PUBLIC
    ${GFXSTREAM_REPO_ROOT}/host
    ${GFXSTREAM_REPO_ROOT}/include
)
