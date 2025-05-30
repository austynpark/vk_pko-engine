#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include "vulkan_types.inl"

/*
	 Staging buffer : Use flag VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT. (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	 Transferred from the GPU to read back on the CPU : Use flag VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT (VK_MEMORY_PROPERTY_HOST_CACHED_BIT)

*/
void vulkan_buffer_create(
	RenderContext* context,
	u64 buffer_size,
	VkBufferUsageFlags buffer_usage_flag,
	VmaMemoryUsage memory_usage_flag,
	VmaAllocationCreateFlags alloc_create_flag,
	Buffer* buffer
);

void vulkan_buffer_copy(RenderContext* context, Buffer* src_buffer, Buffer* dst_buffer, u64 size,u64 src_offset = 0, u64 dst_offset = 0);

void vulkan_buffer_destroy(RenderContext* context, Buffer* buffer);

void vulkan_buffer_upload(RenderContext* context, Buffer* buffer, void* data, u32 data_size);

#endif // !VULKAN_BUFFER_H
