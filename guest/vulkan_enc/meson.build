# Copyright 2022 Android Open Source Project
# SPDX-License-Identifier: MIT

gfxstream_vk_entrypoints = custom_target(
  'gfxstream_vk_entrypoints',
  input : [vk_entrypoints_gen, vk_api_xml],
  output : ['gfxstream_vk_entrypoints.h', 'gfxstream_vk_entrypoints.c'],
  command : [
    prog_python, '@INPUT0@', '--xml', '@INPUT1@', '--proto', '--weak',
    '--out-h', '@OUTPUT0@', '--out-c', '@OUTPUT1@', '--prefix', 'gfxstream_vk',
    '--beta', with_vulkan_beta.to_string()
  ],
)

files_lib_vulkan_enc = files(
  'CommandBufferStagingStream.cpp',
  'DescriptorSetVirtualization.cpp',
  'HostVisibleMemoryVirtualization.cpp',
  'ResourceTracker.cpp',
  'Resources.cpp',
  'Validation.cpp',
  'VkEncoder.cpp',
  'VulkanHandleMapping.cpp',
  'VulkanStreamGuest.cpp',
  'func_table.cpp',
  'goldfish_vk_counting_guest.cpp',
  'goldfish_vk_counting_guest.h',
  'goldfish_vk_deepcopy_guest.cpp',
  'goldfish_vk_extension_structs_guest.cpp',
  'goldfish_vk_marshaling_guest.cpp',
  'goldfish_vk_reserved_marshaling_guest.cpp',
  'goldfish_vk_transform_guest.cpp',
  'gfxstream_vk_private.cpp',
)

lib_vulkan_enc = static_library(
   'vulkan_enc',
   [files_lib_vulkan_enc, gfxstream_vk_entrypoints],
   cpp_args: cpp_args,
   include_directories: [inc_android_emu, inc_guest_iostream, inc_android_compat,
                         inc_opengl_codec, inc_render_enc, inc_system,
                         inc_goldfish_address_space, inc_platform,
                         inc_vulkan_headers, inc_opengl_headers,
                         inc_include_mesa, inc_src],
   link_with: [lib_platform],
   link_args: [vulkan_icd_link_args, ld_args_bsymbolic, ld_args_gc_sections],
   link_depends: vulkan_icd_link_depends,
   dependencies: [dependency('libdrm'), idep_vulkan_runtime_headers,
                  idep_vulkan_util_headers],
)
