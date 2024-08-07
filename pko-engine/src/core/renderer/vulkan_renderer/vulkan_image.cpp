#include "vulkan_image.h"

#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

void vulkan_image_create(
	VulkanContext* context,
	VkImageType image_type,
	u32 width, u32 height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_flags,
	b32 create_view,
	VkImageAspectFlags view_aspect_flags,
	VulkanImage* out_image
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

void vulkan_image_view_create(VulkanContext* context, VkFormat format, VulkanImage* out_image, VkImageAspectFlags aspect_flags)
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

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image)
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

void vulkan_image_layout_transition(
	VulkanImage* image, 
	Command* command,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_flags,
	VkPipelineStageFlags dst_stage_flags
)
{
	//TODO: make configurable
	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier image_barrier_to_transfer = {};
	image_barrier_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	image_barrier_to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_barrier_to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_barrier_to_transfer.image = image->handle;
	image_barrier_to_transfer.subresourceRange = range;

	image_barrier_to_transfer.srcAccessMask = 0;
	image_barrier_to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	//barrier the image into the transfer-receive layout
	vkCmdPipelineBarrier(command->buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_transfer);
}

b8 load_image_from_file(VulkanContext* context, const char* file, VulkanImage* out_image)
{
	i32 tex_width, tex_height, tex_channels;
	stbi_uc* pixels = stbi_load(file, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

	if (!pixels) {
		std::cout << "failed to load texture file " << file << std::endl;
		return false;
	}

	void* pixel_ptr = pixels;

	VulkanBuffer staging_buffer;
	u32 staging_buffer_size = tex_width * tex_height * 4;

	vulkan_buffer_create(context, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &staging_buffer);
	vulkan_buffer_upload(context, &staging_buffer, pixel_ptr, staging_buffer_size);
	stbi_image_free(pixels);

	vulkan_image_create(
		context, VK_IMAGE_TYPE_2D,
		tex_width, tex_height,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		true,
		VK_IMAGE_ASPECT_COLOR_BIT,
		out_image
	);

	// create one time submit command buffer for image copy from staging buffer
	Command one_time_submit;
	vulkan_command_pool_create(context, &one_time_submit, context->device_context.transfer_family.index);
	vulkan_command_buffer_allocate(context, &one_time_submit, true);

	vulkan_command_buffer_begin(&one_time_submit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vulkan_image_layout_transition(
		out_image,
		&one_time_submit,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT
		);

	VkBufferImageCopy copy_region = {};
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;

	//TODO: mipmap
	copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.imageSubresource.mipLevel = 0;
	copy_region.imageSubresource.baseArrayLayer = 0;
	copy_region.imageSubresource.layerCount = 1;
	copy_region.imageExtent = {out_image->width, out_image->height, 1};

	//copy the buffer into the image
	vkCmdCopyBufferToImage(one_time_submit.buffer, staging_buffer.handle, out_image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

	vulkan_image_layout_transition(
		out_image,
		&one_time_submit,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);

	vulkan_command_buffer_end(&one_time_submit);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &one_time_submit.buffer;

	VK_CHECK(vkQueueSubmit(context->device_context.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));
	vkQueueWaitIdle(context->device_context.transfer_queue);
	vulkan_command_pool_destroy(context, &one_time_submit);

	vulkan_buffer_destroy(context, &staging_buffer);

	return true;
}

void vulkan_texture_create(VulkanContext* context, VulkanTexture* out_texture, VulkanImage image, VkFilter filters, VkSamplerAddressMode samplerAdressMode)
{
	out_texture->image = image;

	VkSamplerCreateInfo create_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	create_info.pNext = nullptr;
	create_info.magFilter = filters;
	create_info.minFilter = filters;
	create_info.addressModeU = samplerAdressMode;
	create_info.addressModeV = samplerAdressMode;
	create_info.addressModeW = samplerAdressMode;

	VK_CHECK(vkCreateSampler(context->device_context.handle, &create_info, context->allocator, &out_texture->sampler));
}

void vulkan_texture_destroy(VulkanContext* context, VulkanTexture* texture)
{
	vulkan_image_destroy(context, &texture->image);
	vkDestroySampler(context->device_context.handle, texture->sampler, context->allocator);
}
