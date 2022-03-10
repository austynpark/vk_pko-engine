#include "vulkan_mesh.h"

vulkan_render_object::vulkan_render_object(const char* object_name) : object(object_name)
{
	
}

void vulkan_render_object::upload_mesh(vulkan_context* context)
{
	VkBufferCreateInfo buffer_create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_create_info.size = p_mesh->vertices.size() * sizeof(vertex);
	buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	//VmaAllocationInfo alloc_info{};
	//alloc_info.

	VK_CHECK(vmaCreateBuffer(
		context->vma_allocator,
		&buffer_create_info,
		&alloc_create_info,
		&vertex_buffer.buffer,
		&vertex_buffer.allocation,
		nullptr
	));

	void* data;
	VK_CHECK(vmaMapMemory(context->vma_allocator , vertex_buffer.allocation, &data));

	memcpy(data, p_mesh->vertices.data(), p_mesh->vertices.size() * sizeof(vertex));

	vmaUnmapMemory(context->vma_allocator, vertex_buffer.allocation);

}

void vulkan_render_object::vulkan_render_object_destroy(vulkan_context* context)
{
	vmaDestroyBuffer(context->vma_allocator, vertex_buffer.buffer, vertex_buffer.allocation);
}

vulkan_render_object::~vulkan_render_object()
{
}
