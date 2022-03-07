#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "defines.h"

struct vulkan_context;
struct vulkan_renderpass;

void vulkan_renderpass_create(vulkan_context* context, vulkan_renderpass* renderpass, u32 x, u32 y, u32 w, u32 h);
void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass);

#endif // !VULKAN_RENDERPASS_H
