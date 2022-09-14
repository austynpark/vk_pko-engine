#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

#include "core/renderer/spirv_helper.h"
#include "core/renderer/SPIRV-Reflect/spirv_reflect.h"
#include "core/object.h"

#include "core/file_handle.h"
#include "core/renderer/vulkan_renderer/vulkan_mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>

VkDescriptorPool vulkan_descriptor_pool_create(vulkan_context* context);
VkDescriptorPool get_descriptor_pool(vulkan_context* context ,std::vector<VkDescriptorPool>& pools);
//b8 alloc_descriptor_set(vulkan_context* context ,std::vector<VkDescriptorPool>& pools, VkDescriptorSetLayout* layouts, u32 layout_count);

vulkan_shader::vulkan_shader(vulkan_context* vk_context, vulkan_renderpass* renderpass) : context(vk_context), renderpass(renderpass)
{
}

vulkan_shader::~vulkan_shader()
{
	shutdown();
}

vulkan_shader& vulkan_shader::add_stage(const char* shader_name, VkShaderStageFlagBits stage_flag)
{
	stage_infos.push_back({ shader_name, stage_flag });
	return *this;
}


b8 vulkan_shader::init()
{
	if (reflect_layout(context->device_context.handle) != true)
		return false;
	
	vertex_input_description input_description = vulkan_render_object::get_vertex_input_description();

	if (vulkan_graphics_pipeline_create(context, renderpass, &pipeline,
		stage_infos[0].shader_module, stage_infos[1].shader_module,
		input_description.bindings.size(), input_description.bindings.data(),
		input_description.attributes.size(), input_description.attributes.data(), pipeline.layout
	) != true) {
		return false;
	}

	return true;
}

void vulkan_shader::shutdown()
{
	for (auto& layout : set_layouts) {
		if (layout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(context->device_context.handle, layout, context->allocator);
			layout = VK_NULL_HANDLE;
		}
	}

    vulkan_pipeline_destroy(context, &pipeline);
}

struct descriptor_set_layout_data {
	uint32_t set_number;
	VkDescriptorSetLayoutCreateInfo create_info;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

b8 vulkan_shader::reflect_layout(VkDevice device)
{
	std::vector<descriptor_set_layout_data> set_layouts_data;
	std::vector<VkPushConstantRange> constant_ranges;

	for (auto& s : stage_infos) {

		std::string path = "shader/";
		path.append(s.shader_name);

		std::string buffer;
		read_file(buffer, path);

		if (!SpirvHelper::GLSLtoSPV(s.stage_flag, buffer.c_str(), s.binary_code)) {
			std::cout << "Failed to convert GLSL to Spriv\n";
			return false;
		}
		VkShaderModuleCreateInfo module_create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		module_create_info.codeSize = s.binary_code.size() * sizeof(u32);
		module_create_info.pCode = s.binary_code.data();

		VK_CHECK(vkCreateShaderModule(device, &module_create_info, nullptr, &s.shader_module));

		SpvReflectShaderModule spvmodule;
		SpvReflectResult result = spvReflectCreateShaderModule(s.binary_code.size() * sizeof(uint32_t), s.binary_code.data(), &spvmodule);

		uint32_t count = 0;
		result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectDescriptorSet*> sets(count);
		result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		for (size_t i_set = 0; i_set < sets.size(); ++i_set) {

			const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);

			descriptor_set_layout_data layout = {};

			layout.bindings.resize(refl_set.binding_count);
			for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
				const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
				VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
				layout_binding.binding = refl_binding.binding;
				layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
				layout_binding.descriptorCount = 1;

				for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
					layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
				}
				layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(spvmodule.shader_stage);

				reflected_binding reflected;
				reflected.binding = layout_binding.binding;
				reflected.set = refl_set.set;
				reflected.type = layout_binding.descriptorType;

				bindings[refl_binding.name] = reflected;
			}
			layout.set_number = refl_set.set;
			layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layout.create_info.bindingCount = refl_set.binding_count;
			layout.create_info.pBindings = layout.bindings.data();

			set_layouts_data.push_back(layout);
		}

		//pushconstants	

		result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectBlockVariable*> pconstants(count);
		result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, pconstants.data());
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		if (count > 0) {
			VkPushConstantRange pcs{};
			pcs.offset = pconstants[0]->offset;
			pcs.size = pconstants[0]->size;
			pcs.stageFlags = s.stage_flag;

			constant_ranges.push_back(pcs);
		}
	}

	std::array<descriptor_set_layout_data, 4> merged_layouts;

	for (int i = 0; i < 4; i++) {

		descriptor_set_layout_data& ly = merged_layouts[i];

		ly.set_number = i;

		ly.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		std::unordered_map<int, VkDescriptorSetLayoutBinding> binds;
		for (auto& s : set_layouts_data) {
			if (s.set_number == i) {
				for (auto& b : s.bindings)
				{
					auto it = binds.find(b.binding);
					if (it == binds.end())
					{
						binds[b.binding] = b;
						//ly.bindings.push_back(b);
					}
					else {
						//merge flags
						binds[b.binding].stageFlags |= b.stageFlags;
					}

				}
			}
		}
		for (auto [k, v] : binds)
		{
			ly.bindings.push_back(v);
		}
		//sort the bindings, for hash purposes
		std::sort(ly.bindings.begin(), ly.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
			return a.binding < b.binding;
			});

		ly.create_info.bindingCount = (uint32_t)ly.bindings.size();
		ly.create_info.pBindings = ly.bindings.data();
		ly.create_info.flags = 0;
		ly.create_info.pNext = 0;

		if (ly.create_info.bindingCount > 0) {
			VK_CHECK(vkCreateDescriptorSetLayout(device, &ly.create_info, context->allocator, &set_layouts[i]));
		}
		else {
			set_layouts[i] = VK_NULL_HANDLE;
		}
	}

	//we start from just the default empty pipeline layout info
	VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	pipeline_layout_info.pPushConstantRanges = constant_ranges.data();
	pipeline_layout_info.pushConstantRangeCount = (uint32_t)constant_ranges.size();

	std::array<VkDescriptorSetLayout, 4> compacted_layouts;
	int s = 0;
	for (int i = 0; i < 4; i++) {
		if (set_layouts[i] != VK_NULL_HANDLE) {
			compacted_layouts[s] = set_layouts[i];
			s++;
		}
	}

	pipeline_layout_info.setLayoutCount = s;
	pipeline_layout_info.pSetLayouts = compacted_layouts.data();

	VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, context->allocator, &pipeline.layout));

	return true;
}

b8 vulkan_global_data_initialize(vulkan_context* context, u32 buffer_size)
{
	b8 result = true;
	
	VkDescriptorSetLayoutBinding global_binding{};
	global_binding.binding = 0;
	global_binding.descriptorCount = 1;
	global_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	global_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	global_binding.pImmutableSamplers = VK_NULL_HANDLE;
	
	VkDescriptorSetLayoutCreateInfo global_layout_create_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	global_layout_create_info.bindingCount = 1;
	global_layout_create_info.pBindings = &global_binding;
	//global_layout_create_info.flags

	VK_CHECK(vkCreateDescriptorSetLayout(context->device_context.handle, &global_layout_create_info, context->allocator, &context->global_data.set_layout));

	for (u32 i = 0; i < MAX_FRAME; ++i) {
		//alloc_descriptor_set(context, context->, &context->global_data.set_layout, 1, &context->global_data.ubo_data[i].descriptor_set);
		result = context->dynamic_descriptor_allocators[i].allocate(&context->global_data.ubo_data[i].descriptor_set, context->global_data.set_layout);
		if (!result) {
			std::cout << "global descriptor set allocation failed" << std::endl;
			break;
		}

		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,&context->global_data.ubo_data[i].buffer);

		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = context->global_data.ubo_data[i].buffer.handle;
		buffer_info.offset = 0;
		buffer_info.range = buffer_size;

		VkWriteDescriptorSet write_set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write_set.dstSet = context->global_data.ubo_data[i].descriptor_set;
		write_set.dstBinding = 0;
		write_set.descriptorCount = 1;
		write_set.pBufferInfo = &buffer_info;
		write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    	VkDescriptorSet                  dstSet;
    	uint32_t                         dstBinding;
    	uint32_t                         dstArrayElement;
    	uint32_t                         descriptorCount;
    	VkDescriptorType                 descriptorType;
    	const VkDescriptorImageInfo*     pImageInfo;
    	const VkDescriptorBufferInfo*    pBufferInfo;
    	const VkBufferView*              pTexelBufferView;


		vkUpdateDescriptorSets(context->device_context.handle, 1, &write_set, 0, nullptr);
	}

	return result;
}

void vulkan_global_data_destroy(vulkan_context* context)
{
	for (uint32_t i = 0; i < MAX_FRAME; ++i)
		vulkan_buffer_destroy(context, &context->global_data.ubo_data[i].buffer);
}
