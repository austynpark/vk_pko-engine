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

struct VulkanDevice {
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

struct VulkanCommand {
	VkCommandPool pool;
	VkCommandBuffer buffer;
};

// Samplers should be managed by the texture system / as dyanmic storage
VkSampler sampler;

struct VulkanTexture {
	VkImage image;
	uint32_t aspectMask;
};

struct RenderTarget
{
	VulkanTexture* pTexture;
};

struct VulkanSwapchainSupportInfo {
	VkSurfaceCapabilitiesKHR surface_capabilites;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;

	u32 format_count;
	u32 present_mode_count;
};

struct SwapchainDesc
{
	void* phWnd;
	uint32_t width : 16;
	uint32_t height : 16;
	uint32_t image_count : 2; // MAX_FRAMES (TRIPPLE BUFFERING)

};

struct VulkanSwapchain {
	VkSwapchainKHR handle;
	SwapchainDesc* pDesc;
	
	//VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR image_format;
	u32 image_count;

	u8 max_frames_in_flight;
};

struct VulkanRenderpass {
	VkRenderPass handle;
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct VulkanBuffer {
	VkBuffer handle;
	VmaAllocation allocation;
};

struct VulkanPipeline {
	VkPipeline handle;
	VkPipelineLayout layout;
};

class DescriptorAllocator {
public:
	DescriptorAllocator() = default;

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

class DescriptorLayoutCache {
public:
	DescriptorLayoutCache() = default;
	void init(VkDevice new_device);
	void cleanup();

	VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info);

	struct DescriptorLayoutInfo {
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		bool operator==(const DescriptorLayoutInfo& other) const;

		size_t hash() const;
	};

private:

	struct DescriptorLayoutHash {
		std::size_t operator() (const DescriptorLayoutInfo& info) const {
			return info.hash();
		}
	};

	std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layout_cache;
	VkDevice device = VK_NULL_HANDLE;
};

struct VulkanContext {
	VmaAllocator vma_allocator;
	VkAllocationCallbacks* allocator;
	VkInstance	instance;
	
	//imgui
	// VkDescriptorPool imgui_pool;

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT	debug_messenger;
#endif

	u32 current_frame;

	u32 framebuffer_width;
	u32 framebuffer_height;

	VkSurfaceKHR surface;
	VulkanDevice device_context;
	VulkanSwapchainSupportInfo swapchain_support_info;
	VulkanSwapchain swapchain;
	VulkanRenderpass main_renderpass;

	std::vector<DescriptorAllocator> dynamic_descriptor_allocators;
};
