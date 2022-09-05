#include "vulkan_types.inl"

#include "vulkan_memory_allocate.h"

void vulkan_memory_allocator_create(vulkan_context* context)
{
	VmaVulkanFunctions vulkan_functions = {};
	vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo vma_allocator_create_info{};
	vma_allocator_create_info.physicalDevice = context->device_context.physical_device;
	vma_allocator_create_info.device = context->device_context.handle;
	vma_allocator_create_info.instance = context->instance;
	vma_allocator_create_info.pVulkanFunctions = &vulkan_functions;
	VK_CHECK(vmaCreateAllocator(&vma_allocator_create_info, &context->vma_allocator));
}

void vulkan_memory_allocator_destroy(vulkan_context* context)
{
	vmaDestroyAllocator(context->vma_allocator);
}




