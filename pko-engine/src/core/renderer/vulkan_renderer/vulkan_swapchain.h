#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "defines.h"

struct vulkan_context;
struct vulkan_swapchain;
struct vulkan_swapchain_support_info;

b8 vulkan_swapchain_create(vulkan_context* context, i32 width, i32 height, vulkan_swapchain* out_swapchain);
b8 vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* out_swapchain);
b8 vulkan_swapchain_recreate(vulkan_context* context, i32 width, i32 height);
void vulkan_swapchain_get_support_info(vulkan_context* context, vulkan_swapchain_support_info* out_support_info);

#endif // !VULKAN_SWAPCHAIN_H
