#pragma once

#include "vulkan_functions.h"
#include "defines.h"

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <vector>
#include <cassert>

#define VK_CHECK(call) \
	do {				\
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while(0)


struct vulkan_queue_family {
	u32 index;
	u32 count;
	//std::vector<VkQueue> queues;
};

struct vulkan_device {
	VkDevice handle;
	VkPhysicalDevice physical_device;

	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceProperties properties;

	vulkan_queue_family graphics_family;
	vulkan_queue_family present_family;
	vulkan_queue_family transfer_family;
	vulkan_queue_family compute_family;

	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;

	VkFormat depth_format;
};

struct vulkan_command {
	VkCommandPool pool;
	VkCommandBuffer buffer;
};

struct vulkan_image {
	VkImage handle;
	VkImageView view;
	VmaAllocation allocation; 
	u32 width;
	u32 height;
};

struct vulkan_swapchain_support_info {
	VkSurfaceCapabilitiesKHR surface_capabilites;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;

	u32 format_count;
	u32 present_mode_count;
};

struct vulkan_swapchain {
	VkSwapchainKHR handle;
	VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR image_format;
	u32 image_count;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	vulkan_image depth_attachment;

	u32 image_index;
};

struct vulkan_renderpass {
	VkRenderPass handle;
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct vulkan_pipeline {
	VkPipeline handle;
	VkPipelineLayout layout;
};

struct vulkan_allocated_buffer {
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct vulkan_context {
	VmaAllocator vma_allocator;
	VkAllocationCallbacks* allocator;
	VkInstance					instance;

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT	debug_messenger;
#endif

	VkSurfaceKHR surface;
	vulkan_device device_context;
	vulkan_swapchain_support_info swapchain_support_info;
	vulkan_swapchain swapchain;

	u32 framebuffer_width;
	u32 framebuffer_height;

	vulkan_command graphics_command;
	std::vector<VkFramebuffer>	framebuffers;

	vulkan_renderpass main_renderpass;

	std::vector<VkSemaphore> ready_to_render_semaphores;
	std::vector<VkSemaphore> image_available_semaphores;
	VkFence fence;

	u32 image_index;

	vulkan_pipeline graphics_pipeline;
};
