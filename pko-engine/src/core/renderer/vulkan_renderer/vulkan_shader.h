#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "vulkan_types.inl"

#include <memory>
#include <string>
#include <array>
#include <unordered_map>

//b8 vulkan_shader_module_create(VulkanContext* pContext, VkShaderModule* out_shader_module, const char* path);

// initialize vulkan global descriptor set
b8 vulkan_global_data_initialize(VulkanContext* pContext, u32 buffer_size);
void vulkan_global_data_destroy(VulkanContext* pContext);

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
	vulkan_shader(VulkanContext* pContext, VulkanRenderpass* renderpass);
	~vulkan_shader();

	vulkan_shader& add_stage(const char* shader_name, VkShaderStageFlagBits stage_flag);

	b8 init();
	void shutdown();

	std::unordered_map<std::string, reflected_binding> bindings;
	std::array<VkDescriptorSetLayout, 4> set_layouts;
	Pipeline pipeline;

	b8 reflect_layout(VkDevice device);
private:
	std::vector<stage_info> stage_infos;
	VulkanContext* pContext;
	VulkanRenderpass* renderpass;
};

#endif // !VULKAN_SHADER_H
