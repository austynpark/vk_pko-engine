#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "vulkan_types.inl"
#include "render_type.h"

#include <memory>
#include <string>
#include <array>
#include <unordered_map>

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


inline VkShaderStageFlagBits GetShaderStageVulkan(ShaderStage shaderStage)
{
	if (shaderStage == VERTEX_STAGE)
		return VK_SHADER_STAGE_VERTEX_BIT;
	else if (shaderStage == FRAGMENT_STAGE)
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	else if(shaderStage == COMPUTE_STAGE)
		return VK_SHADER_STAGE_COMPUTE_BIT;

	return VK_SHADER_STAGE_ALL;
}

void vulkan_shader_load(ShaderLoadDesc* loadDesc);

struct VulkanShader {
	const char* name[MAX_SHADER_STAGE];
	VkShaderModule shaderModule[MAX_SHADER_STAGE];
	VkShaderStageFlagBits stageFlagBits[MAX_SHADER_STAGE];
	std::vector<u32> binaryCode[MAX_SHADER_STAGE];
};

class vulkan_shader
{
public:
	vulkan_shader(vulkan_context* context, vulkan_renderpass* renderpass);
	~vulkan_shader();

	vulkan_shader& add_stage(const char* shader_name, VkShaderStageFlagBits stage_flag);

	b8 init();
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
