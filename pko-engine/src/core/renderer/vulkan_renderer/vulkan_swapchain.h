#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "defines.h"
#include "vulkan_types.inl"


b8 vulkan_swapchain_create(vulkan_context* context, i32 width, i32 height, vulkan_swapchain* out_swapchain);
b8 vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* out_swapchain);
b8 vulkan_swapchain_recreate(vulkan_context* context, i32 width, i32 height);
void vulkan_swapchain_get_support_info(vulkan_context* context, vulkan_swapchain_support_info* out_support_info);

b8 acquire_next_image_index_swapchain(
	vulkan_context* context,
	vulkan_swapchain* swapchain,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32* image_index
);

b8 present_image_swapchain(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 current_image_index
);

#endif // !VULKAN_SWAPCHAIN_H
