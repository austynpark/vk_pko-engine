#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "vulkan_types.inl"

void vulkan_rendertarget_create(VulkanContext* context, );



void vulkan_image_create(
	VulkanContext* context,
	VkImageType image_type,
	u32 width,
	u32 height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_flags,
	b32 create_view,
	VkImageAspectFlags view_aspect_flags,
	VulkanImage* out_image
);

void vulkan_image_view_create(
	VulkanContext* context,
	VkFormat format,
	VulkanImage* out_image,
	VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image);

void vulkan_image_layout_transition(VulkanImage* image, 
	Command* command,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_flags,
	VkPipelineStageFlags dst_stage_flags
	);

b8 load_image_from_file(VulkanContext* context, const char* file, VulkanImage* out_image);

void vulkan_texture_create(
	VulkanContext* context,
	VulkanTexture* out_texture,
	VulkanImage image,
	VkFilter filters,
	VkSamplerAddressMode samplerAdressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

void vulkan_texture_destroy(
	VulkanContext* context,
	VulkanTexture* texture
);


#endif // !VULKAN_IMAGE_H
