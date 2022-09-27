#ifndef VULKAN_TEXTURE_MANAGER_H
#define VULKAN_TEXTURE_MANAGER_H

#include "defines.h"

#include "vulkan_types.inl"

#include <unordered_map>
#include <vector>

// key = texture name, value = texture index
struct vulkan_texture_manager {
	std::unordered_map<const char*, u32> texture_map;
	std::vector<vulkan_texture> textures;
};

// return texture id(index of textures)
u32 vulkan_texture_manager_add_texture(vulkan_context* context, const char* texture_name, vulkan_texture new_texture);
void vulkan_texture_manager_cleanup(vulkan_context* context);
const vulkan_texture& vulkan_texture_manager_get(u32 id);
#endif // !VULKAN_TEXTURE_MANAGER_H
