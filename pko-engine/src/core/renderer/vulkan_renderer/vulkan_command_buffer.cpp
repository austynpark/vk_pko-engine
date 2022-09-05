#include "vulkan_command_buffer.h"

#include "vulkan_types.inl"

void vulkan_command_pool_create(vulkan_context* context, vulkan_command* command, u32 queue_family_index)
{
	VkCommandPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = queue_family_index;
	
	VK_CHECK(vkCreateCommandPool(context->device_context.handle, &create_info, context->allocator, &command->pool));
}

void vulkan_command_pool_destroy(vulkan_context* context, vulkan_command* command)
{
	vkDestroyCommandPool(context->device_context.handle, command->pool, context->allocator);
}

void vulkan_command_buffer_allocate(vulkan_context* context, vulkan_command* command, b8 is_primary)
{
	//TODO: multiple command buffer
	VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool = command->pool;
	alloc_info.commandBufferCount = 1;
	alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VK_CHECK(vkAllocateCommandBuffers(context->device_context.handle, &alloc_info, &command->buffer));
}

void vulkan_command_buffer_begin(vulkan_command* command, VkCommandBufferUsageFlags buffer_usage)
{
	if (command == nullptr)
		return;

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags = buffer_usage;
	//TODO: for secondary command buffer
	begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(command->buffer, &begin_info));
}

void vulkan_command_buffer_end(vulkan_command* command)
{
	if (command == nullptr)
		return;

	VK_CHECK(vkEndCommandBuffer(command->buffer));
}
