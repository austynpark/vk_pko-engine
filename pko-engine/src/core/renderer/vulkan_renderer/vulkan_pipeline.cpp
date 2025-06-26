#include "vulkan_pipeline.h"

#include "vulkan_types.inl"
#include <stb_ds.h>

VkShaderStageFlagBits get_vulkan_shader_stage_flag(u32 shader_index)
{
    switch (shader_index)
    {
        case SHADER_STAGE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case SHADER_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case SHADER_STAGE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        default:
            return VK_SHADER_STAGE_ALL_GRAPHICS;
    }

    return VK_SHADER_STAGE_ALL_GRAPHICS;
}

VkPipelineColorBlendAttachmentState get_blend_attachment_state(ColorBlendMode mode)
{
    VkPipelineColorBlendAttachmentState attachment = {};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    switch (mode)
    {
        case COLOR_BLEND_OPAQUE:
            attachment.blendEnable = VK_FALSE;
            break;

        case COLOR_BLEND_ALPHA:
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            attachment.colorBlendOp = VK_BLEND_OP_ADD;

            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;

        case COLOR_BLEND_ADDITIVE:
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.colorBlendOp = VK_BLEND_OP_ADD;

            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;

        case COLOR_BLEND_MULTIPLICATIVE:
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.colorBlendOp = VK_BLEND_OP_ADD;

            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;

            // case BlendMode::PremultipliedAlpha:
            //     attachment.blendEnable = VK_TRUE;
            //     attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            //     attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            //     attachment.colorBlendOp = VK_BLEND_OP_ADD;

            //    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            //    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            //    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            //    break;
    }

    return attachment;
}

void vulkan_pipeline_layout_create(RenderContext* context, Shader* shader,
                                   PipelineLayout** out_layout)
{
    assert(context);
    assert(shader);
    DescriptorSetLayout* layout = (DescriptorSetLayout*)calloc(1, sizeof(DescriptorSetLayout));

    // Unique resource in entire shader stages
    ShaderResource* resources = NULL;

    DescriptorHashMap* descriptor_map = NULL;
    sh_new_arena(descriptor_map);

    for (u32 resource_index = 0; resource_index < shader->resource_count; ++resource_index)
    {
        const ShaderResource& resource = shader->shader_resources[resource_index];

        DescriptorHashMap* node = shgetp_null(descriptor_map, resource.name);
        if (node != NULL)
        {
            assert(node->value < arrlenu(resources));
            resources[node->value].stage_flags |= resource.stage_flags;
            arrpush(resources, resource);
        }
        else
        {
            shput(descriptor_map, resource.name, resource_index);
            arrpush(resources, resource);
        }
    }

    VkDescriptorSetLayoutBinding* binding_map[MAX_DESCRIPTOR_SET_LAYOUT] = {};
    VkPushConstantRange push_constant_ranges[MAX_SHADER_STAGE_COUNT] = {};
    u32 push_constant_count = 0;

    for (u32 resource_index = 0; resource_index < arrlenu(resources); ++resource_index)
    {
        const u32 binding = resources[resource_index].mBinding;
        const u32 set = resources[resource_index].mSet;
        resources[resource_index].name;

        if ((binding != UINT32_MAX) && (set != UINT32_MAX))
        {
            VkDescriptorSetLayoutBinding layout_binding{};
            layout_binding.binding = binding;
            layout_binding.descriptorCount = resources[resource_index].mSize;
            layout_binding.descriptorType = resources[resource_index].type;
            layout_binding.pImmutableSamplers = VK_NULL_HANDLE;
            layout_binding.stageFlags = resources[resource_index].stage_flags;

            arrput(binding_map[set], layout_binding);
        }
        else
        {
            push_constant_ranges[push_constant_count].offset = 0;
            push_constant_ranges[push_constant_count].size = resources[resource_index].mSize;
            push_constant_ranges[push_constant_count].stageFlags =
                resources[resource_index].stage_flags;
            push_constant_count++;
        }
    }

    for (u32 layout_index = 0; layout_index < MAX_DESCRIPTOR_SET_LAYOUT; ++layout_index)
    {
        VkDescriptorSetLayoutBinding* bindings = binding_map[layout_index];

        u32 binding_count = arrlen(bindings);

        if (binding_count != 0)
        {
            VkDescriptorSetLayoutCreateInfo layout_create_info{};
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.pBindings = bindings;
            layout_create_info.bindingCount = binding_count;
            layout_create_info.flags = 0;
            layout_create_info.pNext = VK_NULL_HANDLE;

            VK_CHECK(vkCreateDescriptorSetLayout(context->device_context.handle,
                                                 &layout_create_info, VK_NULL_HANDLE,
                                                 &layout->set_layouts[layout_index]));
        }
    }

    VkDescriptorSetLayout set_layouts[MAX_DESCRIPTOR_SET_LAYOUT];
    u32 set_layout_count = 0;
    for (u32 layout_index = 0; layout_index < MAX_DESCRIPTOR_SET_LAYOUT; ++layout_index)
    {
        if (layout->set_layouts[layout_index])
        {
            set_layouts[set_layout_count++] = layout->set_layouts[layout_index];
        }
    }

    VkPipelineLayoutCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_info.pPushConstantRanges = push_constant_ranges;
    pipeline_info.pushConstantRangeCount = push_constant_count;
    pipeline_info.pSetLayouts = set_layouts;
    pipeline_info.setLayoutCount = set_layout_count;
    pipeline_info.flags = 0;
    pipeline_info.pNext = VK_NULL_HANDLE;

    VK_CHECK(vkCreatePipelineLayout(context->device_context.handle, &pipeline_info, VK_NULL_HANDLE,
                                    &layout->handle));

    layout->descriptor_map = descriptor_map;
    *out_layout = layout;

    arrfree(resources);
}

void vulkan_pipeline_layout_destroy(RenderContext* context, PipelineLayout* layout)
{
    assert(layout);
    assert(context);

    vkDestroyPipelineLayout(context->device_context.handle, layout->handle, nullptr);

    for (u32 i = 0; i < MAX_DESCRIPTOR_SET_LAYOUT; ++i)
    {
        if (layout->set_layouts[i] != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(context->device_context.handle, layout->set_layouts[i],
                                         nullptr);
        }
    }

    shfree(layout->descriptor_map);
}

void vulkan_graphics_pipeline_create(RenderContext* context, PipelineDesc* desc,
                                     Pipeline** out_pipeline)
{
    assert(context);
    assert(desc);
    assert(desc->rasterize_desc);
    assert(desc->depth_stencil_desc);
    assert(desc->layout);
    assert(desc->shader);

    Pipeline* pipeline = (Pipeline*)calloc(1, sizeof(Pipeline));
    VkPipelineRenderingCreateInfoKHR rendering_create_info{};
    rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    // uint32_t viewMask;
    rendering_create_info.colorAttachmentCount = desc->color_attachment_count;
    rendering_create_info.pColorAttachmentFormats = desc->color_attachment_formats;
    rendering_create_info.depthAttachmentFormat = desc->depth_attachment_format;
    rendering_create_info.stencilAttachmentFormat = desc->stencil_attachment_format;

    const Shader* shader = desc->shader;
    VkPipelineShaderStageCreateInfo stage_create_infos[MAX_SHADER_STAGE_COUNT] = {};
    u32 shader_stage_count = 0;

    for (u32 shader_idx = 0; shader_idx < MAX_SHADER_STAGE_COUNT; ++shader_idx)
    {
        if (shader->shader_modules[shader_idx] != NULL)
        {
            stage_create_infos[shader_stage_count].sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage_create_infos[shader_stage_count].pNext = NULL;
            stage_create_infos[shader_stage_count].flags = 0;
            stage_create_infos[shader_stage_count].stage = get_vulkan_shader_stage_flag(shader_idx);
            stage_create_infos[shader_stage_count].module =
                shader->shader_modules[shader_idx]->module;
            stage_create_infos[shader_stage_count].pName = "main";
            stage_create_infos[shader_stage_count++].pSpecializationInfo = NULL;
        }
    }

    const PipelineInputDesc* pipeline_input_desc = desc->input_desc;
    VkVertexInputBindingDescription* vertex_input_binding_descs = NULL;
    VkVertexInputAttributeDescription* vertex_input_attribute_descs = NULL;
    if (pipeline_input_desc->bindings != NULL)
    {
        for (u32 i = 0; i < pipeline_input_desc->binding_count; ++i)
        {
            VkVertexInputBindingDescription binding_desc{};
            binding_desc.binding = pipeline_input_desc->bindings[i].binding;
            binding_desc.inputRate = pipeline_input_desc->bindings[i].input_rate;
            binding_desc.stride = pipeline_input_desc->bindings[i].stride;

            arrput(vertex_input_binding_descs, binding_desc);
        }
    }
    if (pipeline_input_desc->attributes != NULL)
    {
        for (u32 i = 0; i < pipeline_input_desc->attribute_count; ++i)
        {
            VkVertexInputAttributeDescription attribute_desc{};
            attribute_desc.binding = pipeline_input_desc->attributes[i].binding;
            attribute_desc.format = pipeline_input_desc->attributes[i].format;
            attribute_desc.location = pipeline_input_desc->attributes[i].location;
            attribute_desc.offset = pipeline_input_desc->attributes[i].offset;

            arrput(vertex_input_attribute_descs, attribute_desc);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.pNext = NULL;
    vertex_input_create_info.flags = 0;
    vertex_input_create_info.vertexBindingDescriptionCount =
        pipeline_input_desc->bindings ? pipeline_input_desc->binding_count : 0;
    vertex_input_create_info.pVertexBindingDescriptions = vertex_input_binding_descs;
    vertex_input_create_info.vertexAttributeDescriptionCount =
        pipeline_input_desc->attributes ? pipeline_input_desc->attribute_count : 0;
    vertex_input_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descs;

    VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info{};
    assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_state_create_info.pNext = NULL;
    assembly_state_create_info.flags = 0;
    assembly_state_create_info.topology = pipeline_input_desc->topology;
    assembly_state_create_info.primitiveRestartEnable = false;

    VkViewport viewport{};
    viewport.height = 0;
    viewport.width = 0;
    viewport.maxDepth = 0;
    viewport.minDepth = 0;

    VkRect2D scissor{};
    scissor.extent.height = 0;
    scissor.extent.width = 0;
    scissor.offset.x = 0;
    scissor.offset.y = 0;

    VkPipelineViewportStateCreateInfo viewport_create_info{};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.pNext = NULL;
    viewport_create_info.flags = 0;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = &viewport;
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = &scissor;

    const RasterizeDesc* rasterize_desc = desc->rasterize_desc;

    VkPipelineRasterizationStateCreateInfo rasterize_create_info{};
    rasterize_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterize_create_info.pNext = NULL;
    rasterize_create_info.flags = 0;
    rasterize_create_info.depthClampEnable = rasterize_desc->depth_clamp_enable;
    rasterize_create_info.rasterizerDiscardEnable = rasterize_desc->rasterizer_discard_enable;
    rasterize_create_info.polygonMode = rasterize_desc->polygon_mode;
    rasterize_create_info.cullMode = rasterize_desc->cull_mode;
    rasterize_create_info.frontFace = rasterize_desc->front_face;
    rasterize_create_info.depthBiasEnable = rasterize_desc->depth_bias_enable;
    rasterize_create_info.depthBiasConstantFactor = rasterize_desc->depth_bias_constant_factor;
    rasterize_create_info.depthBiasClamp = rasterize_desc->depth_bias_clamp;
    rasterize_create_info.depthBiasSlopeFactor = rasterize_desc->depth_bias_slope_factor;
    rasterize_create_info.lineWidth = rasterize_desc->line_width;

    // TODO: MSAA
    VkPipelineMultisampleStateCreateInfo multisample_create_info{};
    multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_create_info.pNext = NULL;
    multisample_create_info.flags = 0;
    multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_create_info.sampleShadingEnable = VK_FALSE;
    multisample_create_info.minSampleShading = 0;
    multisample_create_info.pSampleMask = NULL;
    multisample_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_create_info.alphaToOneEnable = VK_FALSE;

    const DepthStencilDesc* depth_stencil_desc = desc->depth_stencil_desc;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.pNext = NULL;
    depth_stencil_create_info.flags = 0;
    depth_stencil_create_info.depthTestEnable = depth_stencil_desc->depth_test_enable;
    depth_stencil_create_info.depthWriteEnable = depth_stencil_desc->depth_write_enable;
    depth_stencil_create_info.depthCompareOp = depth_stencil_desc->depth_compare_op;
    depth_stencil_create_info.depthBoundsTestEnable = depth_stencil_desc->depth_bounds_test_enable;
    depth_stencil_create_info.stencilTestEnable = depth_stencil_desc->stencil_test_enable;
    depth_stencil_create_info.front = depth_stencil_desc->stencil_front_op_state;
    depth_stencil_create_info.back = depth_stencil_desc->stencil_back_op_state;
    depth_stencil_create_info.minDepthBounds = depth_stencil_desc->min_depth_bounds;
    depth_stencil_create_info.maxDepthBounds = depth_stencil_desc->max_depth_bounds;

    VkPipelineColorBlendAttachmentState blend_attachment_state[MAX_COLOR_ATTACHMENT]{};

    for (u32 i = 0; i < desc->color_attachment_count; ++i)
    {
        if (desc->blend_modes[i] != NULL)
            blend_attachment_state[i] = get_blend_attachment_state(desc->blend_modes[i]);
        else
            blend_attachment_state[i] = get_blend_attachment_state(desc->blend_modes[0]);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.pNext = NULL;
    color_blend_create_info.flags = 0;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend_create_info.attachmentCount = desc->color_attachment_count;
    color_blend_create_info.pAttachments = blend_attachment_state;
    // color_blend_create_info.blendConstants = {};

    VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = NULL;
    dynamic_state_create_info.flags = 0;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = &rendering_create_info;
    create_info.flags = 0;
    create_info.stageCount = shader_stage_count;
    create_info.pStages = stage_create_infos;
    create_info.pVertexInputState = &vertex_input_create_info;
    create_info.pInputAssemblyState = &assembly_state_create_info;
    create_info.pTessellationState = VK_NULL_HANDLE;
    create_info.pViewportState = &viewport_create_info;
    create_info.pRasterizationState = &rasterize_create_info;
    create_info.pMultisampleState = &multisample_create_info;
    create_info.pDepthStencilState = &depth_stencil_create_info;
    create_info.pColorBlendState = &color_blend_create_info;
    create_info.pDynamicState = &dynamic_state_create_info;
    create_info.layout = desc->layout->handle;
    create_info.renderPass = VK_NULL_HANDLE;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = 0;

    vkCreateGraphicsPipelines(context->device_context.handle, VK_NULL_HANDLE, 1, &create_info,
                              nullptr, &pipeline->handle);

    *out_pipeline = pipeline;

    arrfree(vertex_input_binding_descs);
    arrfree(vertex_input_attribute_descs);
}

void vulkan_graphics_pipeline_destroy(RenderContext* context, Pipeline* pipeline)
{
    vkDestroyPipeline(context->device_context.handle, pipeline->handle, NULL);
    pipeline->handle = VK_NULL_HANDLE;
}
