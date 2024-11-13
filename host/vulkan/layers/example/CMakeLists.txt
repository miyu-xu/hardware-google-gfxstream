add_library(
    VkLayer_gfxstream_example
    SHARED
    example-layer.cpp)
target_link_libraries(
    VkLayer_gfxstream_example
    PUBLIC
    gfxstream_vulkan_headers)