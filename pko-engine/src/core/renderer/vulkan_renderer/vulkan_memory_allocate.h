#ifndef VULKAN_MEMORY_ALLOCATE_H
#define VULKAN_MEMORY_ALLOCATE_H

#include "defines.h"

struct VulkanContext;

void vulkan_memory_allocator_create(VulkanContext* pContext);
void vulkan_memory_allocator_destroy(VulkanContext* pContext);


#endif // !VULKAN_MEMORY_ALLOCATE_H
