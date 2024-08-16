#include "vulkan_buffer.h"

#include "vulkan_command_buffer.h"

void vulkan_buffer_create(
	VulkanContext* pContext,
	u64 buffer_size,
	VkBufferUsageFlags buffer_usage_flag,
	VmaMemoryUsage memory_usage_flag,
	VmaAllocationCreateFlags alloc_create_flag,
	VulkanBuffer* buffer
	) 
{
	VkBufferCreateInfo buffer_create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_create_info.size = buffer_size;
	buffer_create_info.usage = buffer_usage_flag;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = memory_usage_flag;
	alloc_create_info.flags = alloc_create_flag;

	VK_CHECK(vmaCreateBuffer(
		pContext->vma_allocator,
		&buffer_create_info,
		&alloc_create_info,
		&buffer->handle,
		&buffer->allocation,
		nullptr
	));

}

void vulkan_buffer_copy(VulkanContext* pContext,
	VulkanBuffer* src_buffer,
	VulkanBuffer* dst_buffer,
	u64 size,
	u64 src_offset,
	u64 dst_offset
)
{
	Command one_time_submit;
	vulkan_command_pool_create(pContext, &one_time_submit, pContext->device_context.transfer_family.index);
	vulkan_command_buffer_allocate(pContext, &one_time_submit, true);

	VkBufferCopy buffer_copy{
		src_offset,// srcOffset
		dst_offset,// dstOffset
		size// size
	};

	vulkan_command_buffer_begin(&one_time_submit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkCmdCopyBuffer(one_time_submit.buffer, src_buffer->handle, dst_buffer->handle, 1, &buffer_copy);
	vulkan_command_buffer_end(&one_time_submit);
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &one_time_submit.buffer;

	VK_CHECK(vkQueueSubmit(pContext->device_context.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));
	vkQueueWaitIdle(pContext->device_context.transfer_queue);
	vulkan_command_pool_destroy(pContext, &one_time_submit);
}


void vulkan_buffer_destroy(VulkanContext* pContext, VulkanBuffer* buffer) {

	vmaDestroyBuffer(pContext->vma_allocator, buffer->handle, buffer->allocation);
}

void vulkan_buffer_upload(VulkanContext* pContext, VulkanBuffer* buffer, void* data, u32 data_size)
{
	void* copied_data;
	vmaMapMemory(pContext->vma_allocator, buffer->allocation, &copied_data);
	memcpy(copied_data, data, data_size);
	vmaUnmapMemory(pContext->vma_allocator, buffer->allocation);
}
