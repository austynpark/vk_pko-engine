#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "vulkan_types.inl"

void vulkan_image_create(
	RenderContext* context,
	VkImageType image_type,
	u32 width,
	u32 height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_flags,
	b32 create_view,
	VkImageAspectFlags view_aspect_flags,
	Image* out_image
);

void vulkan_image_view_create(
	RenderContext* context,
	VkFormat format,
	Image* out_image,
	VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(RenderContext* context, Image* image);

void vulkan_image_layout_transition(Image* image, 
	vulkan_command* command,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_flags,
	VkPipelineStageFlags dst_stage_flags
	);

b8 load_image_from_file(RenderContext* context, const char* file, Image* out_image);

void vulkan_texture_create(
	RenderContext* context,
	vulkan_texture* out_texture,
	Image image,
	VkFilter filters,
	VkSamplerAddressMode samplerAdressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

void vulkan_texture_destroy(
	RenderContext* context,
	vulkan_texture* texture
);


#endif // !VULKAN_IMAGE_H
