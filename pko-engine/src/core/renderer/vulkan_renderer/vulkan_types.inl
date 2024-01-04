#pragma once

#include "vulkan_functions.h"
#include "defines.h"

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <vector>
#include <unordered_map>
#include <cassert>

#define VK_CHECK(call) \
	do {				\
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while(0)

constexpr int MAX_FRAME = 3;

struct QueueFamily {
	u32 index = -1;
	u32 count;
	//std::vector<VkQueue> queues;
};

struct Device {
	VkDevice handle;
	VkPhysicalDevice physical_device;

	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceProperties properties;

	QueueFamily graphics_family;
	QueueFamily present_family;
	QueueFamily transfer_family;
	QueueFamily compute_family;

	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;

	VkFormat depth_format;
};

struct Image {
	VkImage handle;
	VkImageView view;
	VmaAllocation allocation; 
	u32 width;
	u32 height;
};

struct SwapchainSupportInfo {
	VkSurfaceCapabilitiesKHR surface_capabilites;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;

	u32 format_count;
	u32 present_mode_count;
};

struct Swapchain {
	VkSwapchainKHR handle;
	VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR image_format;
	u32 image_count;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	Image depth_attachment;
	u8 max_frames_in_flight;
};

struct Buffer {
	VkBuffer handle;
	VmaAllocation allocation;
};

struct Pipeline {
	VkPipeline handle;
	VkPipelineLayout layout;
};

struct RenderContext {
	VmaAllocator vma_allocator;
	VkAllocationCallbacks* allocator;
	VkInstance	instance;
	
	//imgui
	VkDescriptorPool imgui_pool;

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT	debug_messenger;
#endif

	u32 current_frame;

	u32 framebuffer_width;
	u32 framebuffer_height;

	VkSurfaceKHR surface;
	Device device_context;
	SwapchainSupportInfo swapchain_support_info;
	Swapchain swapchain;
};
