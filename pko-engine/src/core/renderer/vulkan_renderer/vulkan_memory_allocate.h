#ifndef VULKAN_MEMORY_ALLOCATE_H
#define VULKAN_MEMORY_ALLOCATE_H

#include "defines.h"

struct RenderContext;

void vulkan_memory_allocator_create(RenderContext* context);
void vulkan_memory_allocator_destroy(RenderContext* context);


#endif // !VULKAN_MEMORY_ALLOCATE_H
