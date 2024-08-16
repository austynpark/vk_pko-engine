#include "vulkan_types.inl"

#include "vulkan_memory_allocate.h"

void vulkan_memory_allocator_create(VulkanContext* pContext)
{
	VmaVulkanFunctions vulkan_functions = {};
	vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo vma_allocator_create_info{};
	vma_allocator_create_info.physicalDevice = pContext->device_context.physical_device;
	vma_allocator_create_info.device = pContext->device_context.handle;
	vma_allocator_create_info.instance = pContext->instance;
	vma_allocator_create_info.pVulkanFunctions = &vulkan_functions;
	VK_CHECK(vmaCreateAllocator(&vma_allocator_create_info, &pContext->vma_allocator));
}

void vulkan_memory_allocator_destroy(VulkanContext* pContext)
{
	vmaDestroyAllocator(pContext->vma_allocator);
}




