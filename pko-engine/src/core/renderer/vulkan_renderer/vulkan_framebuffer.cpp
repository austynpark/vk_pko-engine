#include "vulkan_framebuffer.h"

#include <iostream>

void vulkan_framebuffer_create(vulkan_context* context, vulkan_renderpass* renderpass, u32 attachment_count, VkImageView* image_view, VkFramebuffer* out_framebuffer)
{
	VkFramebufferCreateInfo create_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	create_info.renderPass = renderpass->handle;
	create_info.attachmentCount = attachment_count;
	create_info.pAttachments = image_view;
	create_info.width = context->framebuffer_width;
	create_info.height = context->framebuffer_height;
	create_info.layers = 1;

	VK_CHECK(vkCreateFramebuffer(
		context->device_context.handle,
		&create_info,
		context->allocator,
		out_framebuffer
	));
}

void vulkan_framebuffer_destroy(vulkan_context* context, VkFramebuffer* framebuffer) {
	vkDestroyFramebuffer(context->device_context.handle, *framebuffer, context->allocator);
	framebuffer = 0;
}
