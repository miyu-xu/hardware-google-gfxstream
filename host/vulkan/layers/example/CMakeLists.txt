add_library(
    VkLayer_gfxstream_example
    SHARED
    example-layer.cpp)
target_link_libraries(
    VkLayer_gfxstream_example
    PUBLIC
    gfxstream_vulkan_headers)
add_custom_command(
    TARGET VkLayer_gfxstream_example
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_LIST_DIR}/VkLayer_gfxstream_example.json
    $<TARGET_FILE_DIR:VkLayer_gfxstream_example>/VkLayer_gfxstream_example.json)