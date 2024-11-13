# Vulkan Layer Example

This simple vulkan layer example shows how to create a very minimal custom vulkan layer and how the
vulkan loader finds and uses it.

For more detailed information, see
https://vulkan.lunarg.com/doc/view/latest/linux/loader_and_layer_interface.html, specifically, the
section titled [***Layer Dispatch Initialization***](https://vulkan.lunarg.com/doc/view/latest/linux/loader_and_layer_interface.html#user-content-layer-dispatch-initialization).

# High-Level Overview

This code defines a Vulkan layer that primarily focuses on intercepting `vkCreateInstance` and
`vkGetInstanceProcAddr` calls.

1. **`vkCreateInstance` Interception:**
   - Intercepts `vkCreateInstance` calls from the application.
   - Retrieves the next layer's `vkGetInstanceProcAddr` and `vkCreateInstance`.
   - Calls the next layer's `vkCreateInstance` to create the instance.
   - Stores the new instance's information in the `sInstanceMap`.

2. **`vkGetInstanceProcAddr` Interception:**
   - Intercepts `vkGetInstanceProcAddr` calls from the application.
   - If the requested function is `vkCreateInstance`, returns the layer's own implementation.
   - Otherwise, retrieves the next layer's `vkGetInstanceProcAddr` from `sInstanceMap` and calls it.

# How To Run

To run, you'll need to define two environment variables when running your binary:
- `VK_LAYER_PATH`: the path to the layer manifest files (`VkLayer_gfxstream_example.json`).
- `LD_LIBRARY_PATH`: append the path to where the layer library is
(`libVkLayer_gfxstream_example.so`).
- `VK_INSTANCE_LAYERS`: the layer names to enable (`VK_LAYER_gfxstream_example`).