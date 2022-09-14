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

struct vulkan_queue_family {
	u32 index = -1;
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

struct vulkan_texture {
	vulkan_image image;
	VkSampler sampler;
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
	u8 max_frames_in_flight;
};

struct vulkan_renderpass {
	VkRenderPass handle;
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct vulkan_allocated_buffer {
	VkBuffer handle;
	VmaAllocation allocation;
};

struct vulkan_pipeline {
	VkPipeline handle;
	VkPipelineLayout layout;
};

struct vulkan_uniform_buffer_data {
	vulkan_allocated_buffer buffer;
	VkDescriptorSet descriptor_set;
} typedef vulkan_ubo_data;

struct vulkan_global_data {
	VkDescriptorSetLayout set_layout;

	// per frame data ( MAX FRAME  = 3, triple buffering )
	vulkan_ubo_data ubo_data[MAX_FRAME];
};

class descriptor_allocator {
public:
	descriptor_allocator() = default;

	struct pool_sizes {
		std::vector<std::pair<VkDescriptorType, float>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
		};

		VkDescriptorPool create_pool(VkDevice device, i32 count, VkDescriptorPoolCreateFlags flags);
	};

	void reset_pools();
	bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

	void init(VkDevice new_device);

	void cleanup();

	VkDevice device = VK_NULL_HANDLE;
private:
	VkDescriptorPool grab_pool();

	VkDescriptorPool current_pool{ VK_NULL_HANDLE };
	pool_sizes descriptor_sizes;
	std::vector<VkDescriptorPool> used_pools;
	std::vector<VkDescriptorPool> free_pools;
};

class descriptor_layout_cache {
public:
	descriptor_layout_cache() = default;
	void init(VkDevice new_device);
	void cleanup();

	VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info);

	struct descriptor_layout_info {
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		bool operator==(const descriptor_layout_info& other) const;

		size_t hash() const;
	};

private:

	struct descriptor_layout_hash {
		std::size_t operator() (const descriptor_layout_info& info) const {
			return info.hash();
		}
	};

	std::unordered_map<descriptor_layout_info, VkDescriptorSetLayout, descriptor_layout_hash> layout_cache;
	VkDevice device = VK_NULL_HANDLE;
};

struct vulkan_context {
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
	vulkan_device device_context;
	vulkan_swapchain_support_info swapchain_support_info;
	vulkan_swapchain swapchain;
	vulkan_renderpass main_renderpass;

	std::vector<VkSemaphore> ready_to_render_semaphores;
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkFence> render_fences;
	std::vector<descriptor_allocator> dynamic_descriptor_allocators;

	vulkan_global_data global_data;

	//TODO: maybe put in scene.h? (not sure)
	VkDescriptorSetLayout object_set_layout;
};
