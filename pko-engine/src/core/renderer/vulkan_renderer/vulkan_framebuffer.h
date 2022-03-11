#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include "vulkan_types.inl"

void vulkan_framebuffer_create(vulkan_context* context, vulkan_renderpass* renderpass, u32 attachment_count, VkImageView* image_view, VkFramebuffer* out_framebuffer);
void vulkan_framebuffer_destroy(vulkan_context* context, VkFramebuffer* framebuffer);
#endif // !VULKAN_FRAMEBUFFER_H
