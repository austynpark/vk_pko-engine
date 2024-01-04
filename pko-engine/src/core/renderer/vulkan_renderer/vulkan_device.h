#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "defines.h"

struct Device;
struct RenderContext;

b8 vulkan_device_create(RenderContext* context,Device* device_context);
b8 vulkan_device_destroy(RenderContext* context, Device* device_context);

void vulkan_get_device_queue(Device* device_context);
b8 vulkan_device_detect_depth_format(Device* device);
#endif // !VULKAN_DEVICE_H

