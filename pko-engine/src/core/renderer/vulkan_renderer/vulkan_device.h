#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "defines.h"

struct VulkanDevice;
struct VulkanContext;

b8 vulkan_device_create(VulkanContext* context,VulkanDevice* device_context);
b8 vulkan_device_destroy(VulkanContext* context, VulkanDevice* device_context);

void vulkan_get_device_queue(VulkanDevice* device_context);
b8 vulkan_device_detect_depth_format(VulkanDevice* device);
#endif // !VULKAN_DEVICE_H

