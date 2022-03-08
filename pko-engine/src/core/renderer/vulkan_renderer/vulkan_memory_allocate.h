#ifndef VULKAN_MEMORY_ALLOCATE_H
#define VULKAN_MEMORY_ALLOCATE_H

#include "defines.h"

struct vulkan_context;

void vulkan_memory_allocator_create(vulkan_context* context);
void vulkan_memory_allocator_destroy(vulkan_context* context);

#endif // !VULKAN_MEMORY_ALLOCATE_H
