#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "defines.h"

#include "vulkan_types.inl"

bool is_image_format_depth_only(VkFormat format);
bool is_image_format_depth_stencil(VkFormat format);
VkSampleCountFlagBits to_vulkan_sample_count(u32 sampleCount);
VkImageUsageFlags descriptor_type_to_vulkan_image_usage(VkDescriptorType type);
VkImageUsageFlags resource_state_to_vulkan_image_usage(ResourceState state);
VkImageAspectFlags format_to_vulkan_image_aspect(VkFormat format);
VkImageLayout resource_state_to_vulkan_image_layout(ResourceState state);

void vulkan_texture_create(RenderContext* context, TextureDesc* desc, Texture** textures);

void vulkan_texture_destroy(RenderContext* context, Texture* texture);

b8 vulkan_rendertarget_create(RenderContext* context, RenderTargetDesc* desc,
                              RenderTarget** out_render_target);
void vulkan_rendertarget_destroy(RenderContext* context, RenderTarget* render_target);

b8 vulkan_swapchain_create(RenderContext* context, const SwapchainDesc* desc,
                           VulkanSwapchain** swapchain);
b8 vulkan_swapchain_destroy(RenderContext* context, VulkanSwapchain* swapchain);
b8 vulkan_swapchain_recreate(RenderContext* context, VulkanSwapchain** out_swapchain);
void vulkan_swapchain_get_support_info(RenderContext* context,
                                       VulkanSwapchainSupportInfo* out_support_info);

b8 load_image_from_file(RenderContext* context, const char* pFile, Texture* texture);

b8 acquire_next_image_index_swapchain(RenderContext* context, VulkanSwapchain* swapchain,
                                      u64 timeout_ns, VkSemaphore semaphore, VkFence fence,
                                      u32* image_index);

b8 present_image_swapchain(VulkanContext* context, VulkanSwapchain* swapchain,
                           VkQueue present_queue, VkSemaphore render_complete_semaphore,
                           u32 current_image_index);

#endif  // !VULKAN_IMAGE_H
