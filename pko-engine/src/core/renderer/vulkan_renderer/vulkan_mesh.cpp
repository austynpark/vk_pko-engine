#include "vulkan_mesh.h"

#include "vulkan_buffer.h"

vulkan_render_object::vulkan_render_object(const char* object_name) : object(object_name)
{
	
}

void vulkan_render_object::upload_mesh(vulkan_context* context)
{
	vulkan_allocated_buffer staging_buffer;

	vulkan_buffer_create(context, p_mesh->vertices.size() * sizeof(vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &staging_buffer);
	
	void* data;
	VK_CHECK(vmaMapMemory(context->vma_allocator , staging_buffer.allocation, &data));

	memcpy(data, p_mesh->vertices.data(), p_mesh->vertices.size() * sizeof(vertex));

	vmaUnmapMemory(context->vma_allocator, staging_buffer.allocation);

	vulkan_buffer_create(context, p_mesh->vertices.size() * sizeof(vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, &vertex_buffer);

	vulkan_buffer_copy(context, &staging_buffer, &vertex_buffer, p_mesh->vertices.size() * sizeof(vertex));
}

void vulkan_render_object::vulkan_render_object_destroy(vulkan_context* context)
{
	vmaDestroyBuffer(context->vma_allocator, vertex_buffer.handle, vertex_buffer.allocation);
}

vulkan_render_object::~vulkan_render_object()
{
}

vertex_input_description vulkan_render_object::get_vertex_input_description()
{
	vertex_input_description result;

	VkVertexInputBindingDescription input_binding_description;
	input_binding_description.binding = 0;
	input_binding_description.stride = sizeof(vertex);
	input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	result.bindings.push_back(input_binding_description);

	std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions(3);

	input_attribute_descriptions[0].binding = 0;
	input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	input_attribute_descriptions[0].location = 0;
	input_attribute_descriptions[0].offset = offsetof(vertex, position);

	input_attribute_descriptions[1].binding = 0;
	input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	input_attribute_descriptions[1].location = 1;
	input_attribute_descriptions[1].offset = offsetof(vertex, normal);

	input_attribute_descriptions[2].binding = 0;
	input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	input_attribute_descriptions[2].location = 2;
	input_attribute_descriptions[2].offset = offsetof(vertex, uv);

	result.attributes = input_attribute_descriptions;

	return result;
}
