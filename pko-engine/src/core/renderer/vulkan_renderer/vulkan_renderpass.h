#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "defines.h"

struct VulkanContext;
struct VulkanRenderpass;

void vulkan_renderpass_create(VulkanContext* context, VulkanRenderpass* renderpass, u32 x, u32 y, u32 w, u32 h);
void vulkan_renderpass_destroy(VulkanContext* context, VulkanRenderpass* renderpass);

#endif // !VULKAN_RENDERPASS_H
