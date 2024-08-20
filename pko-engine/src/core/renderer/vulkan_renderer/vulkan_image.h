#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "defines.h"

#include "vulkan_types.inl"

struct SwapchainDesc;

bool is_image_format_depth_only(VkFormat format);
bool is_image_format_depth_stencil(VkFormat format);
VkSampleCountFlagBits to_vulkan_sample_count(u32 sampleCount);
VkImageUsageFlags descriptor_type_to_vulkan_image_usage(DescriptorType type);
VkImageUsageFlags resource_state_to_vulkan_image_usage(ResourceState state);
VkImageAspectFlags format_to_vulkan_image_aspect(VkFormat format);
VkImageLayout resource_state_to_vulkan_image_layout(ResourceState state);

b8 vulkan_texture_create(
	VulkanContext* pContext,
	TextureDesc* pDesc,
	Texture** ppTexture
);

void vulkan_texture_destroy(
	VulkanContext* pContext,
	Texture* pTexture
);

b8 vulkan_rendertarget_create(VulkanContext* pContext, RenderTargetDesc* pRenderTargetDesc, RenderTarget** ppRenderTarget);
void vulkan_rendertarget_destroy(VulkanContext* pContext, RenderTarget* pRenderTarget);

b8 vulkan_swapchain_create(VulkanContext* pContext, const SwapchainDesc* desc, VulkanSwapchain** out_swapchain);
b8 vulkan_swapchain_destroy(VulkanContext* pContext, VulkanSwapchain* out_swapchain);
b8 vulkan_swapchain_recreate(VulkanContext* pContext, i32 width, i32 height);
void vulkan_swapchain_get_support_info(VulkanContext* pContext, VulkanSwapchainSupportInfo* out_support_info);

void vulkan_framebuffer_create(VulkanContext* pContext, VulkanRenderpass* renderpass, u32 attachment_count, VkImageView* image_view, VkFramebuffer* out_framebuffer);
void vulkan_framebuffer_destroy(VulkanContext* pContext, VkFramebuffer* framebuffer);

b8 load_image_from_file(VulkanContext* pContext, const char* pFile, Texture* pTexture);

b8 acquire_next_image_index_swapchain(
	VulkanContext* pContext,
	VulkanSwapchain* swapchain,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32* image_index
);

b8 present_image_swapchain(
    VulkanContext* pContext,
    VulkanSwapchain* swapchain,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 current_image_index
);

#endif // !VULKAN_IMAGE_H
