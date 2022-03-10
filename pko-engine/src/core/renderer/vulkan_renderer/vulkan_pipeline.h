#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "vulkan_types.inl"

b8 vulkan_graphics_pipeline_create(
	vulkan_context* context,
	vulkan_renderpass* renderpass,
	vulkan_pipeline* out_pipeline,
	u32 binding_description_count,
	VkVertexInputBindingDescription* binding_descriptions,
	u32 attribute_description_count,
	VkVertexInputAttributeDescription* attribute_descriptions,
	u32 push_constant_range_count,
	VkPushConstantRange* push_constant_range,
	const char* vertex_file_path,
	const char* fragment_file_path
);

void vulkan_pipeline_destroy(
	vulkan_context* context,
	vulkan_pipeline* pipeline
);

void vulkan_pipeline_bind(vulkan_command* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline);

#endif // !VULKAN_PIPELINE_H
