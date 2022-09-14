#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "vulkan_types.inl"

void vulkan_image_create(
	vulkan_context* context,
	VkImageType image_type,
	u32 width,
	u32 height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_flags,
	b32 create_view,
	VkImageAspectFlags view_aspect_flags,
	vulkan_image* out_image
);

void vulkan_image_view_create(
	vulkan_context* context,
	VkFormat format,
	vulkan_image* out_image,
	VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);

void vulkan_image_layout_transition(vulkan_image* image, 
	vulkan_command* command,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_flags,
	VkPipelineStageFlags dst_stage_flags
	);

b8 load_image_from_file(vulkan_context* context, const char* file, vulkan_image* out_image);

void vulkan_texture_create(
	vulkan_context* context,
	vulkan_texture* out_texture,
	vulkan_image image,
	VkFilter filters,
	VkSamplerAddressMode samplerAdressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

void vulkan_texture_destroy(
	vulkan_context* context,
	vulkan_texture* texture
);


#endif // !VULKAN_IMAGE_H
