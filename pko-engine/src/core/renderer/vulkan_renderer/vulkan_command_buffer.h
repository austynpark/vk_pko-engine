#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

struct vulkan_context;
struct vulkan_command;

void vulkan_command_pool_create(vulkan_context* context, vulkan_command* command, u32 queue_family_index);
void vulkan_command_pool_destroy(vulkan_context* context, vulkan_command* command);

void vulkan_command_buffer_allocate(vulkan_context* context, vulkan_command* command, b8 is_primary, u32 command_count);
void vulkan_command_buffer_begin(VkCommandBuffer cmdBuff, b8 oneTimeSubmit);
void vulkan_command_buffer_end(VkCommandBuffer cmdBuff);

#endif // !VULKAN_COMMAND_BUFFER_H
