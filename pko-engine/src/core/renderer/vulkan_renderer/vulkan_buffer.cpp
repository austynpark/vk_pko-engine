#include "vulkan_buffer.h"

void vulkan_buffer_create(
	vulkan_context* context,
	u64 buffer_size,
	VkBufferUsageFlagBits buffer_usage_flag,
	VmaMemoryUsage memory_usage_flag,
	vulkan_allocated_buffer* buffer
	) 
{
	VkBufferCreateInfo buffer_create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_create_info.size = buffer_size;
	buffer_create_info.usage = buffer_usage_flag;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = memory_usage_flag;

	VK_CHECK(vmaCreateBuffer(
		context->vma_allocator,
		&buffer_create_info,
		&alloc_create_info,
		&buffer->handle,
		&buffer->allocation,
		nullptr
	));

}

void vulkan_buffer_copy(vulkan_context* context,
	vulkan_allocated_buffer* src_buffer,
	vulkan_allocated_buffer* dst_buffer,
	u64 size,
	u64 src_offset,
	u64 dst_offset
)
{
	VkBufferCopy buffer_copy{
		src_offset,// srcOffset
		dst_offset,// dstOffset
		size// size
	};



}


void vulkan_buffer_destroy(vulkan_context* context, vulkan_allocated_buffer* buffer) {

	vmaDestroyBuffer(context->vma_allocator, buffer->handle, buffer->allocation);
}

void vulkan_buffer_upload(vulkan_context* context, vulkan_allocated_buffer* buffer, void* data, u32 data_size)
{
	void* copied_data;
	vmaMapMemory(context->vma_allocator, buffer->allocation, &copied_data);
	memcpy(copied_data, data, data_size);
	vmaUnmapMemory(context->vma_allocator, buffer->allocation);
}
