#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "defines.h"

struct DeviceContext;
struct VulkanContext;

b8 vulkan_device_create(VulkanContext* context,DeviceContext* device_context);
b8 vulkan_device_destroy(VulkanContext* context, DeviceContext* device_context);

void vulkan_get_device_queue(DeviceContext* device_context);
b8 vulkan_device_detect_depth_format(DeviceContext* device);
#endif // !VULKAN_DEVICE_H

