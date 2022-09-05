#ifndef VULKAN_DESCRIPTOR_ALLOCATOR
#define VULKAN_DESCRIPTOR_ALLOCATOR

#include "vulkan_types.inl"

#include <vector>
#include <unordered_map>

class descriptor_allocator {
public:

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

	VkDevice device;
private:
	VkDescriptorPool grab_pool();

	VkDescriptorPool current_pool{ VK_NULL_HANDLE };
	pool_sizes descriptor_sizes;
	std::vector<VkDescriptorPool> used_pools;
	std::vector<VkDescriptorPool> free_pools;
};

class descriptor_layout_cache {
public:
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
	VkDevice device;
};

#endif // !VULKAN_DESCRIPTOR_ALLOCATOR
