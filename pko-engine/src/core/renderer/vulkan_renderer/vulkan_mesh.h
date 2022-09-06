#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_types.inl"
#include "core/object.h"

struct vertex_input_description {
	std::vector<VkVertexInputAttributeDescription> attributes;
	std::vector<VkVertexInputBindingDescription> bindings;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

class vulkan_render_object : public object
{
public:
	vulkan_render_object() = delete;
	vulkan_render_object(const char* object_name);
	void upload_mesh(vulkan_context* context);
	void vulkan_render_object_destroy(vulkan_context* context);
	~vulkan_render_object() override;

	static vertex_input_description get_vertex_input_description();

	vulkan_allocated_buffer vertex_buffer;
};


#endif // !VULKAN_MESH_H