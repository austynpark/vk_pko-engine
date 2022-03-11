#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "defines.h"

struct vulkan_device;
struct vulkan_context;

b8 vulkan_device_create(vulkan_context* context,vulkan_device* device_context);
b8 vulkan_device_destroy(vulkan_context* context, vulkan_device* device_context);

void vulkan_get_device_queue(vulkan_device* device_context);
b8 vulkan_device_detect_depth_format(vulkan_device* device);
#endif // !VULKAN_DEVICE_H

