#ifndef VULKAN_MEMORY_ALLOCATE_H
#define VULKAN_MEMORY_ALLOCATE_H

#include "defines.h"

struct VulkanContext;

void vulkan_memory_allocator_create(VulkanContext* context);
void vulkan_memory_allocator_destroy(VulkanContext* context);


#endif // !VULKAN_MEMORY_ALLOCATE_H
