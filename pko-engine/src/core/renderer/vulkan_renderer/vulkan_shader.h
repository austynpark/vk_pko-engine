#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "vulkan_types.inl"

#include <memory>
#include <string>
#include <array>
#include <unordered_map>

struct vertex_input_description; // vulkan_mesh.h

//b8 vulkan_shader_module_create(vulkan_context* context, VkShaderModule* out_shader_module, const char* path);

// initialize vulkan global descriptor set
b8 vulkan_global_data_initialize(vulkan_context* context, u32 buffer_size);
void vulkan_global_data_destroy(vulkan_context* context);

struct stage_info {
	const char* shader_name;
	VkShaderStageFlagBits stage_flag;
	VkShaderModule shader_module;
	std::vector<u32> binary_code;
};

struct reflected_binding {
	uint32_t set;
	uint32_t binding;
	VkDescriptorType type;
};

class vulkan_shader
{
public:
	vulkan_shader(vulkan_context* context, vulkan_renderpass* renderpass);
	~vulkan_shader();

	vulkan_shader& add_stage(const char* shader_name, VkShaderStageFlagBits stage_flag);

	b8 init(
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		b8 depth_enable = true,
		vertex_input_description* input_description = nullptr
	);
	void shutdown();

	std::unordered_map<std::string, reflected_binding> bindings;
	std::array<VkDescriptorSetLayout, 4> set_layouts;
	vulkan_pipeline pipeline;

	b8 reflect_layout(VkDevice device);
private:
	std::vector<stage_info> stage_infos;
	vulkan_context* context;
	vulkan_renderpass* renderpass;
};

#endif // !VULKAN_SHADER_H
