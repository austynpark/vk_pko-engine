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

typedef union ClearValue
{
	struct
	{
		float r;
		float g;
		float b;
		float a;
	};
	struct
	{
		float    depth;
		uint32_t stencil;
	};
}ClearValue;

typedef struct QueueFamily {
	u32 index = -1;
	u32 count;
	//std::vector<VkQueue> queues;
}QueueFamily;

typedef struct DeviceContext {
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
}DeviceContext;

typedef struct Command {
	VkCommandPool pool;
	VkCommandBuffer buffer;
}Command;

// Samplers should be managed by the texture system / as dynamic storage
VkSampler sampler;

typedef enum ResourceState
{
	RESOURCE_STATE_UNDEFINED = 0,
	RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
	RESOURCE_STATE_INDEX_BUFFER = 0x2,
	RESOURCE_STATE_RENDER_TARGET = 0x4,
	RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
	RESOURCE_STATE_DEPTH_WRITE = 0x10,
	RESOURCE_STATE_DEPTH_READ = 0x20,
	RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
	RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
	RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
	RESOURCE_STATE_PRESENT = 0x200,
	RESOURCE_STATE_COPY_DEST = 0x400,
	RESOURCE_STATE_COPY_SOURCE = 0x800,
	RESOURCE_STATE_COMMON = 0x1000
}ResourceState;

typedef enum DescriptorType
{
	DEXCRIPTOR,
	DESCRIPTOR_TYPE_RENDERTARGET,
	DESCRIPTOR_TYPE_TEXTURE,
	DESCRIPTOR_TYPE_RW_TEXTURE,
	DESCRIPTOR_TYPE_
}DescriptorType;

typedef struct TextureDesc
{
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
	uint32_t sampleCount;

	ResourceState startState;
	VkDescriptorType 
}TextureDesc;

typedef struct VulkanTexture {
	VkImage pSRVDescriptor;
	VkImage* pUAVDescriptors;

	uint32_t width : 16;
	uint32_t height : 16;
	uint32_t format : 8;
	uint32_t sampleCount : 4;
	uint32_t aspectMask : 4;
}VulkanTexture;

typedef struct RenderTargetDesc {
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
	VkFormat format;
	
	// For Descriptor 
	VkDescriptorType descriptorType;
	const void* pNativeHandle; // VkImage

}RenderTargetDesc;

typedef struct RenderTarget
{
	VulkanTexture* pTexture;

}RenderTarget;

typedef struct VulkanSwapchainSupportInfo {
	VkSurfaceCapabilitiesKHR surface_capabilites;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;

	u32 format_count;
	u32 present_mode_count;
}VulkanSwapchainSupportInfo;

typedef struct SwapchainDesc
{
	void* phWnd;
	uint32_t width : 16;
	uint32_t height : 16;
	uint32_t image_count : 2; // MAX_FRAMES (TRIPPLE BUFFERING)
}SwapchainDesc;

typedef struct VulkanSwapchain {
	RenderTarget** ppRenderTargets;

	VkSwapchainKHR handle;
	SwapchainDesc* pDesc;
	
	//VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR image_format;
	u32 image_count;

	u8 max_frames_in_flight;
}VulkanSwapchain;

typedef struct VulkanRenderpass {
	VkRenderPass handle;
	u32 x;
	u32 y;
	u32 width;
	u32 height;
}VulkanRenderpass;

typedef struct VulkanBuffer {
	VkBuffer handle;
	VmaAllocation allocation;
}VulkanBuffer;

typedef struct VulkanPipeline {
	VkPipeline handle;
	VkPipelineLayout layout;
}VulkanPipeline;

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

typedef struct VulkanContext {
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
	DeviceContext device_context;
	VulkanSwapchainSupportInfo swapchain_support_info;
	VulkanRenderpass main_renderpass;

	DescriptorAllocator* pDynamic_descriptor_allocators;
} VulkanContext;
