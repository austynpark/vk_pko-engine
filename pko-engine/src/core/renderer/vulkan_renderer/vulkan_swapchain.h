#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "vulkan_types.inl"

b8 vulkan_swapchain_create(RenderContext* context, i32 width, i32 height, Swapchain* out_swapchain);
b8 vulkan_swapchain_destroy(RenderContext* context, Swapchain* out_swapchain);
b8 vulkan_swapchain_recreate(RenderContext* context, i32 width, i32 height);
void vulkan_swapchain_get_support_info(RenderContext* context, SwapchainSupportInfo* out_support_info);

b8 acquire_next_image_index_swapchain(
	RenderContext* context,
	Swapchain* swapchain,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32* image_index
);

b8 present_image_swapchain(
    RenderContext* context,
    Swapchain* swapchain,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 current_image_index
);

#endif // !VULKAN_SWAPCHAIN_H
