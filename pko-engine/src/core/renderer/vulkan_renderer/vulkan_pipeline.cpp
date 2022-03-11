#include "vulkan_pipeline.h"

#include "core/file_handle.h"
#include <iostream>

b8 vulkan_shader_module_create(vulkan_context* context, VkShaderModule* out_shader_module, const char* path);

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
)
{
	VkShaderModule vertex_shader_module;

	if (!vulkan_shader_module_create(context, &vertex_shader_module, vertex_file_path)) {
		std::cout << " vertex shader module failed to create" << std::endl;
		return false;
	}

	VkShaderModule fragment_shader_module;

	if (!vulkan_shader_module_create(context, &fragment_shader_module, fragment_file_path)) {
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
	VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
	dynamic_state_info.pDynamicStates = dynamic_states;

	VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.pushConstantRangeCount = push_constant_range_count;
	pipeline_layout_info.pPushConstantRanges = push_constant_range;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = 0;

	VK_CHECK(vkCreatePipelineLayout(context->device_context.handle, &pipeline_layout_info, context->allocator, &out_pipeline->layout));

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
	//graphics_pipeline_create_info.pDynamicState = &dynamic_state_info;
	graphics_pipeline_create_info.layout = out_pipeline->layout;
	graphics_pipeline_create_info.renderPass = renderpass->handle;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;
	
	VK_CHECK(vkCreateGraphicsPipelines(context->device_context.handle, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, context->allocator, &out_pipeline->handle));

	vkDestroyShaderModule(context->device_context.handle, vertex_shader_module, context->allocator);
	vkDestroyShaderModule(context->device_context.handle, fragment_shader_module, context->allocator);

	return true;
}

void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline)
{
	//vkQueueWaitIdle(context->device_context.graphics_queue);

	vkDestroyPipelineLayout(context->device_context.handle, pipeline->layout, context->allocator);
	vkDestroyPipeline(context->device_context.handle, pipeline->handle, context->allocator);
}

void vulkan_pipeline_bind(vulkan_command* command_buffer,VkPipelineBindPoint bind_point ,vulkan_pipeline* pipeline) {
	vkCmdBindPipeline(command_buffer->buffer, bind_point, pipeline->handle);
}

b8 vulkan_shader_module_create(vulkan_context* context, VkShaderModule* out_shader_module, const char* path)
{
	file_handle file;
	if (!pko_file_read(path, &file)) {
		std::cout << "failed to read " << path << std::endl;
		return false;
	}
	VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.pCode = (uint32_t*)file.str;
	create_info.codeSize = file.size;

	VK_CHECK(vkCreateShaderModule(context->device_context.handle, &create_info, context->allocator, out_shader_module));

	pko_file_close(&file);


	return true;
}