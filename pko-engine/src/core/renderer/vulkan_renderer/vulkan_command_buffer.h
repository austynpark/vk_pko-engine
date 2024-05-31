#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

struct VulkanContext;
struct VulkanCommand;

void vulkan_command_pool_create(VulkanContext* context, VulkanCommand* command, u32 queue_family_index);
void vulkan_command_pool_destroy(VulkanContext* context, VulkanCommand* command);

void vulkan_command_buffer_allocate(VulkanContext* context, VulkanCommand* command, b8 is_primary);
void vulkan_command_buffer_begin(VulkanCommand* command, VkCommandBufferUsageFlags buffer_usage);
void vulkan_command_buffer_end(VulkanCommand* command);

#endif // !VULKAN_COMMAND_BUFFER_H
