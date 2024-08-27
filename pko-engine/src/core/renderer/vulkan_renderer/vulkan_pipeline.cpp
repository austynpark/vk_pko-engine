#include "vulkan_pipeline.h"

#include "core/file_handle.h"
#include <iostream>

b8 vulkan_shader_module_create(VulkanContext* pContext, VkShaderModule* out_shader_module, const char* path);

b8 vulkan_graphics_pipeline_create(
	VulkanContext* pContext,
	VulkanRenderpass* renderpass,
	Pipeline* out_pipeline,
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
)
{
	VkShaderModule vertex_shader_module;

	if (!vulkan_shader_module_create(pContext, &vertex_shader_module, vertex_file_path)) {
		std::cout << " vertex shader module failed to create" << std::endl;
		return false;
	}

	VkShaderModule fragment_shader_module;

	if (!vulkan_shader_module_create(pContext, &fragment_shader_module, fragment_file_path)) {
		std::cout << "frag shader module failed to create" << std::endl;
		return false;
	}

	VkPipelineShaderStageCreateInfo vert_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_create_info.module = vertex_shader_module;
	vert_create_info.pName = "main";
	//vert_create_info.pSpecializationInfo: allows you to specify values for shader constants

	VkPipelineShaderStageCreateInfo frag_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_create_info.module = fragment_shader_module;
	frag_create_info.pName = "main";
	//frag_create_info.pSpecializationInfo: allows you to specify values for shader constants

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_create_info,
		frag_create_info
	};

	// format of the vertex data
	VkPipelineVertexInputStateCreateInfo vert_input_info{};
	vert_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vert_input_info.vertexBindingDescriptionCount = binding_description_count;
	vert_input_info.pVertexBindingDescriptions = binding_descriptions;
	vert_input_info.vertexAttributeDescriptionCount = attribute_description_count;
	vert_input_info.pVertexAttributeDescriptions = attribute_descriptions;
 
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = renderpass->x;
	viewport.y = renderpass->y;
	viewport.width = renderpass->width;
	viewport.height = renderpass->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = renderpass->width;
	scissor.extent.height = renderpass->height;

	VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_info.viewportCount = 1;
	viewport_info.pViewports = &viewport;
	viewport_info.scissorCount = 1;
	viewport_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// TODO: depth attachment
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.minDepthBounds = 0.0f; // Optional
	depth_stencil_info.maxDepthBounds = 1.0f; // Optional
	depth_stencil_info.stencilTestEnable = VK_FALSE;
	depth_stencil_info.front = {}; // Optional
	depth_stencil_info.back = {}; // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	
	VkPipelineColorBlendStateCreateInfo color_blend_info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_info.logicOpEnable = VK_FALSE;
	color_blend_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;
	
	VkDynamicState dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
	dynamic_state_info.pDynamicStates = dynamic_states;

	VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.pushConstantRangeCount = push_constant_range_count;
	pipeline_layout_info.pPushConstantRanges = push_constant_range;
	pipeline_layout_info.setLayoutCount = descriptor_set_layout_count;
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts;

	VK_CHECK(vkCreatePipelineLayout(pContext->device_context.handle, &pipeline_layout_info, pContext->allocator, &out_pipeline->layout));

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	//graphics_pipeline_create_info.flags
	graphics_pipeline_create_info.stageCount = sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo);
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vert_input_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_info;
	graphics_pipeline_create_info.pTessellationState = 0;
	graphics_pipeline_create_info.pViewportState = &viewport_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterizer;
	graphics_pipeline_create_info.pMultisampleState = &multisampling;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_info;
	graphics_pipeline_create_info.pColorBlendState = &color_blend_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_info;
	graphics_pipeline_create_info.layout = out_pipeline->layout;
	graphics_pipeline_create_info.renderPass = renderpass->handle;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;
	
	VK_CHECK(vkCreateGraphicsPipelines(pContext->device_context.handle, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, pContext->allocator, &out_pipeline->handle));

	vkDestroyShaderModule(pContext->device_context.handle, vertex_shader_module, pContext->allocator);
	vkDestroyShaderModule(pContext->device_context.handle, fragment_shader_module, pContext->allocator);

	return true;
}

b8 vulkan_graphics_pipeline_create(VulkanContext* pContext, VulkanRenderpass* renderpass, Pipeline* out_pipeline, VkShaderModule vertex_shader_module,
	VkShaderModule fragment_shader_module, u32 binding_description_count, VkVertexInputBindingDescription* binding_descriptions, u32 attribute_description_count, VkVertexInputAttributeDescription* attribute_descriptions, VkPipelineLayout pipeline_layout)
{
	VkPipelineShaderStageCreateInfo vert_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_create_info.module = vertex_shader_module;
	vert_create_info.pName = "main";
	//vert_create_info.pSpecializationInfo: allows you to specify values for shader constants

	VkPipelineShaderStageCreateInfo frag_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_create_info.module = fragment_shader_module;
	frag_create_info.pName = "main";
	//frag_create_info.pSpecializationInfo: allows you to specify values for shader constants

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_create_info,
		frag_create_info
	};

	// format of the vertex data
	VkPipelineVertexInputStateCreateInfo vert_input_info{};
	vert_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vert_input_info.vertexBindingDescriptionCount = binding_description_count;
	vert_input_info.pVertexBindingDescriptions = binding_descriptions;
	vert_input_info.vertexAttributeDescriptionCount = attribute_description_count;
	vert_input_info.pVertexAttributeDescriptions = attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = renderpass->x;
	viewport.y = renderpass->y;
	viewport.width = renderpass->width;
	viewport.height = renderpass->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = renderpass->width;
	scissor.extent.height = renderpass->height;

	VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_info.viewportCount = 1;
	viewport_info.pViewports = &viewport;
	viewport_info.scissorCount = 1;
	viewport_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// TODO: depth attachment
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.minDepthBounds = 0.0f; // Optional
	depth_stencil_info.maxDepthBounds = 1.0f; // Optional
	depth_stencil_info.stencilTestEnable = VK_FALSE;
	depth_stencil_info.front = {}; // Optional
	depth_stencil_info.back = {}; // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo color_blend_info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_info.logicOpEnable = VK_FALSE;
	color_blend_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;

	VkDynamicState dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
	dynamic_state_info.pDynamicStates = dynamic_states;

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	//graphics_pipeline_create_info.flags
	graphics_pipeline_create_info.stageCount = sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo);
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vert_input_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_info;
	graphics_pipeline_create_info.pTessellationState = 0;
	graphics_pipeline_create_info.pViewportState = &viewport_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterizer;
	graphics_pipeline_create_info.pMultisampleState = &multisampling;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_info;
	graphics_pipeline_create_info.pColorBlendState = &color_blend_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_info;
	graphics_pipeline_create_info.layout = out_pipeline->layout;
	graphics_pipeline_create_info.renderPass = renderpass->handle;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	VK_CHECK(vkCreateGraphicsPipelines(pContext->device_context.handle, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, pContext->allocator, &out_pipeline->handle));

	vkDestroyShaderModule(pContext->device_context.handle, vertex_shader_module, pContext->allocator);
	vkDestroyShaderModule(pContext->device_context.handle, fragment_shader_module, pContext->allocator);

	return true;
}

void vulkan_pipeline_destroy(VulkanContext* pContext, Pipeline* pipeline)
{
	//vkQueueWaitIdle(pContext->device_context.mGraphicsQueue);

	if (pipeline->layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(pContext->device_context.handle, pipeline->layout, pContext->allocator);
		pipeline->layout = VK_NULL_HANDLE;
	}

	if (pipeline->handle != VK_NULL_HANDLE) {
		vkDestroyPipeline(pContext->device_context.handle, pipeline->handle, pContext->allocator);
		pipeline->handle = VK_NULL_HANDLE;
	}
}

void vulkan_pipeline_bind(Command* command_buffer,VkPipelineBindPoint bind_point ,Pipeline* pipeline) {
	//TODO: command buffer
	vkCmdBindPipeline(command_buffer->buffer, bind_point, pipeline->handle);
}

b8 vulkan_shader_module_create(VulkanContext* pContext, VkShaderModule* out_shader_module, const char* path)
{
	file_handle file;
	if (!pko_file_read(path, &file)) {
		std::cout << "failed to read " << path << std::endl;
		return false;
	}
	VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.pCode = (uint32_t*)file.str;
	create_info.codeSize = file.size;

	VK_CHECK(vkCreateShaderModule(pContext->device_context.handle, &create_info, pContext->allocator, out_shader_module));

	pko_file_close(&file);

	return true;
}

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shader_module)
{
	VkPipelineShaderStageCreateInfo shader_stage_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shader_stage_create_info.stage = stage;
	shader_stage_create_info.module = shader_module;
	shader_stage_create_info.pName = "main";

	return shader_stage_create_info;
}

VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	pipeline_input_assembly_state_create_info.topology = topology;
	pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	return pipeline_input_assembly_state_create_info;
}

VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(VkPolygonMode polygon_mode)
{
	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
	pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;

	pipeline_rasterization_state_create_info.polygonMode = polygon_mode;
	pipeline_rasterization_state_create_info.lineWidth = 1.0f;
	pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	pipeline_rasterization_state_create_info.depthBiasClamp = 0.f;
	pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.f;

	return pipeline_rasterization_state_create_info;
}

VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info()
{
	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipeline_multisample_state_create_info.minSampleShading = 1.0f;
	pipeline_multisample_state_create_info.pSampleMask = nullptr;
	pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	return pipeline_multisample_state_create_info;
}

VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state()
{
	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{};
	pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
	return pipeline_color_blend_attachment_state;
}

VkPipelineLayoutCreateInfo pipeline_layout_create_info(VkDescriptorSetLayout* p_set_layouts, uint32_t layout_count, VkPushConstantRange* p_constant_ranges, uint32_t constant_count)
{
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	pipeline_layout_create_info.flags = 0;
	pipeline_layout_create_info.setLayoutCount = layout_count;
	pipeline_layout_create_info.pSetLayouts = p_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = constant_count;
	pipeline_layout_create_info.pPushConstantRanges = p_constant_ranges;

	return pipeline_layout_create_info;
}

VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool b_depth_test, bool b_depth_write, VkCompareOp compare_op)
{
	VkPipelineDepthStencilStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.depthTestEnable = b_depth_test ? VK_TRUE : VK_FALSE;
	info.depthWriteEnable = b_depth_write ? VK_TRUE : VK_FALSE;
	info.depthCompareOp = b_depth_test ? compare_op : VK_COMPARE_OP_ALWAYS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f; // Optional
	info.maxDepthBounds = 1.0f; // Optional
	info.stencilTestEnable = VK_FALSE;

	return info;
}
