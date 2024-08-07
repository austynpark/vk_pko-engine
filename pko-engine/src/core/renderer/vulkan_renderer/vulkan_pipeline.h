#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "vulkan_types.inl"

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shader_module);
VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(VkPrimitiveTopology topology);
VkPipelineRasterizationStateCreateInfo  pipeline_rasterization_state_create_info(VkPolygonMode polygon_mode);
VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info();
VkPipelineColorBlendAttachmentState  pipeline_color_blend_attachment_state();
VkPipelineLayoutCreateInfo pipeline_layout_create_info(VkDescriptorSetLayout* p_set_layouts, uint32_t layout_count, VkPushConstantRange* p_constant_ranges, uint32_t constant_count);
VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool b_depth_test, bool b_depth_write, VkCompareOp compare_op);


b8 vulkan_graphics_pipeline_create(
	VulkanContext* context,
	VulkanRenderpass* renderpass,
	VulkanPipeline* out_pipeline,
	u32 binding_description_count,
	VkVertexInputBindingDescription* binding_descriptions,
	u32 attribute_description_count,
	VkVertexInputAttributeDescription* attribute_descriptions,
	u32 push_constant_range_count,
	VkPushConstantRange* push_constant_range,
	u32 descriptor_set_layout_count,
	VkDescriptorSetLayout* descriptor_set_layouts,
	const char* vertex_file_path,
	const char* fragment_file_path
);

b8 vulkan_graphics_pipeline_create(
	VulkanContext* context,
	VulkanRenderpass* renderpass,
	VulkanPipeline* out_pipeline,
	VkShaderModule vertex_shader_module,
	VkShaderModule fragment_shader_module,
	u32 binding_description_count,
	VkVertexInputBindingDescription* binding_descriptions,
	u32 attribute_description_count,
	VkVertexInputAttributeDescription* attribute_descriptions,
	VkPipelineLayout pipeline_layout
);

void vulkan_pipeline_destroy(
	VulkanContext* context,
	VulkanPipeline* pipeline
);

void vulkan_pipeline_bind(Command* command_buffer, VkPipelineBindPoint bind_point, VulkanPipeline* pipeline);

#endif // !VULKAN_PIPELINE_H
