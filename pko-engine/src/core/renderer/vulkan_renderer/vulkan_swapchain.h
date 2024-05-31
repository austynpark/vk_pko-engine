#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "vulkan_types.inl"

struct SwapchainDesc;

b8 vulkan_swapchain_create(VulkanContext* context, const SwapchainDesc* desc, VulkanSwapchain** out_swapchain);
b8 vulkan_swapchain_destroy(VulkanContext* context, VulkanSwapchain* out_swapchain);
b8 vulkan_swapchain_recreate(VulkanContext* context, i32 width, i32 height);
void vulkan_swapchain_get_support_info(VulkanContext* context, VulkanSwapchainSupportInfo* out_support_info);

b8 acquire_next_image_index_swapchain(
	VulkanContext* context,
	VulkanSwapchain* swapchain,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32* image_index
);

b8 present_image_swapchain(
    VulkanContext* context,
    VulkanSwapchain* swapchain,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 current_image_index
);

#endif // !VULKAN_SWAPCHAIN_H
