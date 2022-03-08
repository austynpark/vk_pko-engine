#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "vulkan_types.inl"

b8 vulkan_graphics_pipeline_create(
	vulkan_context* context,
	vulkan_renderpass* renderpass,
	vulkan_pipeline* out_pipeline,
	const char* vertex_file_path,
	const char* fragment_file_path
);

void vulkan_pipeline_destroy(
	vulkan_context* context,
	vulkan_pipeline* pipeline
);

void vulkan_pipeline_bind(vulkan_command* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline);

#endif // !VULKAN_PIPELINE_H
