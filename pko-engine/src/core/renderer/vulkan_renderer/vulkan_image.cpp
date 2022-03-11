#include "vulkan_image.h"

void vulkan_image_create(
	vulkan_context* context,
	VkImageType image_type,
	u32 width, u32 height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_flags,
	b32 create_view,
	VkImageAspectFlags view_aspect_flags,
	vulkan_image* out_image
)
{
	out_image->width = width;
	out_image->height = width;

	VkImageCreateInfo image_create_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	image_create_info.imageType = image_type;
	image_create_info.format = format;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1; //TODO: Support configurable depth
	image_create_info.mipLevels = 4;	//TODO: Support mip mapping
	image_create_info.arrayLayers = 1;	//TODO: Support number of layers in the image
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = tiling;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	alloc_create_info.requiredFlags = memory_flags;

	//allocate and create the image
	VK_CHECK(vmaCreateImage(context->vma_allocator,&image_create_info, &alloc_create_info, &out_image->handle, &out_image->allocation, nullptr));

	if (create_view) {
		vulkan_image_view_create(context, format, out_image, view_aspect_flags);
	}
}

void vulkan_image_view_create(vulkan_context* context, VkFormat format, vulkan_image* out_image, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo image_view_create_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	image_view_create_info.image = out_image->handle;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; //TODO: Make configurable
	image_view_create_info.format = format;
	//image_view_create_info.components
	image_view_create_info.subresourceRange.aspectMask = aspect_flags;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(context->device_context.handle, &image_view_create_info, context->allocator, &out_image->view));
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image)
{
	if (image->view) {
		vkDestroyImageView(context->device_context.handle, image->view, context->allocator);
		image->view = VK_NULL_HANDLE;
	}

	if (image->allocation) {
		vmaFreeMemory(context->vma_allocator, image->allocation);
		image->allocation = VK_NULL_HANDLE;
	}

	if (image->handle) {
		vmaDestroyImage(context->vma_allocator, image->handle, image->allocation);
		image->handle = VK_NULL_HANDLE;
	}
}
