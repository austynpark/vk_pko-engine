#include "vulkan_command_buffer.h"

#include "vulkan_types.inl"

void vulkan_command_pool_create(VulkanContext* pContext, Command* command, u32 queue_family_index)
{
	VkCommandPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = queue_family_index;
	
	VK_CHECK(vkCreateCommandPool(pContext->device_context.handle, &create_info, pContext->allocator, &command->pool));
}

void vulkan_command_pool_destroy(VulkanContext* pContext, Command* command)
{
	vkDestroyCommandPool(pContext->device_context.handle, command->pool, pContext->allocator);
}

void vulkan_command_buffer_allocate(VulkanContext* pContext, Command* command, b8 is_primary)
{
	VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool = command->pool;
	alloc_info.commandBufferCount = 1;
	alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VK_CHECK(vkAllocateCommandBuffers(pContext->device_context.handle, &alloc_info, &command->buffer));
}

void vulkan_command_buffer_begin(Command* command, VkCommandBufferUsageFlags buffer_usage)
{
	if (command == nullptr)
		return;

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags = buffer_usage;
	//TODO: for secondary command buffer
	begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(command->buffer, &begin_info));
}

void vulkan_command_buffer_end(Command* command)
{
	if (command == nullptr)
		return;

	VK_CHECK(vkEndCommandBuffer(command->buffer));
}
