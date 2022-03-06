#pragma once

#include "vulkan_functions.h"

#include <vector>
#include <cassert>

#define VK_CHECK(call) \
	do {				\
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while(0)

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

	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
};

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

};

struct vulkan_context {
	VkAllocationCallbacks* allocator;
	VkInstance					instance;

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT	debug_messenger;
#endif

	VkSurfaceKHR surface;
	vulkan_device device_context;
	vulkan_swapchain_support_info swapchain_support_info;
	vulkan_swapchain swapchain;
};
