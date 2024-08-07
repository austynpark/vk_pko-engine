#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

struct VulkanContext;
struct Command;

void vulkan_command_pool_create(VulkanContext* context, Command* command, u32 queue_family_index);
void vulkan_command_pool_destroy(VulkanContext* context, Command* command);

void vulkan_command_buffer_allocate(VulkanContext* context, Command* command, b8 is_primary);
void vulkan_command_buffer_begin(Command* command, VkCommandBufferUsageFlags buffer_usage);
void vulkan_command_buffer_end(Command* command);

#endif // !VULKAN_COMMAND_BUFFER_H
