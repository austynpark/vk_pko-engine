#include "vulkan_texture_manager.h"

#include "vulkan_image.h"

static vulkan_texture_manager texture_manager;

u32 vulkan_texture_manager_add_texture(vulkan_context* context, const char* name, vulkan_texture texture)
{
	u32 index = 0;
	// not found! add new texture
	if (texture_manager.texture_map.find(name) == texture_manager.texture_map.end()) {
		index = texture_manager.textures.size();
		texture_manager.textures.push_back(texture);
		texture_manager.texture_map[name] = index;
	}
	else {
		index = texture_manager.texture_map[name];
	}

	return index;
}

void vulkan_texture_manager_cleanup(vulkan_context* context)
{
	for (auto texture : texture_manager.textures) {
		vulkan_texture_destroy(context, &texture);
	}
}

const vulkan_texture& vulkan_texture_manager_get(u32 id)
{
	return texture_manager.textures[id];
}
