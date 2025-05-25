#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "defines.h"

struct DeviceContext;
struct RenderContext;

b8 vulkan_device_create(RenderContext* context,DeviceContext* device_context);
b8 vulkan_device_destroy(RenderContext* context, DeviceContext* device_context);

void vulkan_get_device_queue(DeviceContext* device_context);
b8 vulkan_device_detect_depth_format(DeviceContext* device_context);
#endif // !VULKAN_DEVICE_H

