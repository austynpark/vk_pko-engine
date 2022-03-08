#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "vulkan_types.inl"

b8 vulkan_shader_module_create(vulkan_context* context, VkShaderModule* out_shader_module, const char* path);

#endif // !VULKAN_SHADER_H
