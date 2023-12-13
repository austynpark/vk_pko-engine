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

void vulkan_command_buffer_allocate(vulkan_context* context, vulkan_command* command, b8 is_primary, u32 command_count)
{
	command->cmdCount = command_count;

	for (u32 cmd = 0; cmd < command_count; ++cmd)
	{
		VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandPool = command->pool;
		alloc_info.commandBufferCount = 1;
		alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		VK_CHECK(vkAllocateCommandBuffers(context->device_context.handle, &alloc_info, &command->buffer[cmd]));
	}
}

void vulkan_command_buffer_begin(VkCommandBuffer cmdBuff, b8 oneTimeSubmit)
{
	if (cmdBuff == VK_NULL_HANDLE)
		return;

	//TODO: check cmdBuff state and call vkResetCommandBuffer if needed

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags = oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	//TODO: for secondary command buffer
	begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(cmdBuff, &begin_info));
}

void vulkan_command_buffer_end(VkCommandBuffer cmdBuff)
{
	if (cmdBuff == VK_NULL_HANDLE)
		return;

	VK_CHECK(vkEndCommandBuffer(cmdBuff));
}
