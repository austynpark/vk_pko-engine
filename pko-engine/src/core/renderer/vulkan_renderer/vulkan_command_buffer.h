#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

void vulkan_command_pool_create(vulkan_context* context, vulkan_command* command, u32 queue_family_index);
void vulkan_command_pool_destroy(vulkan_context* context, vulkan_command* command);

void vulkan_command_buffer_allocate(vulkan_context* context, vulkan_command* command, b8 is_primary);

#endif // !VULKAN_COMMAND_BUFFER_H
