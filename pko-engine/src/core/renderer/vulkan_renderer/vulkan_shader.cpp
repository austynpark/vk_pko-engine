#include "vulkan_shader.h"

#include "core/file_handle.h"
#include <iostream>

b8 vulkan_shader_module_create(vulkan_context* context, VkShaderModule* out_shader_module, const char* path)
{
	file_handle file;
	if (!pko_file_read(path, &file)) {
		std::cout << "failed to read " << path << std::endl;
		return false;
	}
	VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.pCode = (u32*)file.str;
	create_info.codeSize = file.size;

	pko_file_close(&file);

	VK_CHECK(vkCreateShaderModule(context->device_context.handle, &create_info, context->allocator, out_shader_module));

	return true;
}