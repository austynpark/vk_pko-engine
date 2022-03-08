#include "vulkan_memory_allocate.h"

#include "vulkan_types.inl"

void vulkan_memory_allocator_create(vulkan_context* context)
{
	VmaAllocatorCreateInfo vma_allocator_create_info{};
	vma_allocator_create_info.physicalDevice = context->device_context.physical_device;
	vma_allocator_create_info.device = context->device_context.handle;
	vma_allocator_create_info.instance = context->instance;
	VK_CHECK(vmaCreateAllocator(&vma_allocator_create_info, &context->vma_allocator));
}

void vulkan_memory_allocator_destroy(vulkan_context* context)
{
	vmaDestroyAllocator(context->vma_allocator);
}
