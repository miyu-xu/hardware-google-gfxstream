add_library(
    VkLayer_gfxstream_unique_handle
    SHARED
    VkLayer_unique_handle.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_gfxstream_unique_handle.json)

if (LINUX)
target_compile_options(
    VkLayer_gfxstream_unique_handle
    PRIVATE
    -DVK_ENABLE_BETA_EXTENSIONS
    -DVK_USE_PLATFORM_XCB_KHR
    -DVK_USE_PLATFORM_XLIB_KHR
    -DVK_USE_PLATFORM_XLIB_XRANDR_EXT
    -DVK_USE_PLATFORM_WAYLAND_KHR
)
endif()

target_link_libraries(
    VkLayer_gfxstream_unique_handle
    PUBLIC
    OpenglRender_vulkan_cereal
    gfxstream_vulkan_headers)
target_include_directories(
    VkLayer_gfxstream_unique_handle
    PUBLIC
    ${GFXSTREAM_REPO_ROOT}/host/vulkan
    ${GFXSTREAM_REPO_ROOT}/host/vulkan/cereal/common
    ${CMAKE_CURRENT_LIST_DIR}/include)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_gfxstream_unique_handle.json
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_LIST_DIR}/VkLayer_gfxstream_unique_handle.json
    $<TARGET_FILE_DIR:VkLayer_gfxstream_unique_handle>/VkLayer_gfxstream_unique_handle.json)