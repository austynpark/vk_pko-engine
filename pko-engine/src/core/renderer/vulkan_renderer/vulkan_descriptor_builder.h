#ifndef VULKAN_DESCRIPTOR_BUILDER
#define VULKAN_DESCRIPTOR_BUILDER

#include "vulkan_types.inl"

#include <memory>

class descriptor_builder {
public:
	void begin(VkDevice device);

	descriptor_builder& bind_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
	descriptor_builder& bind_image(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

	b8 build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
	b8 build(VkDescriptorSet& set);
private:

	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	std::unique_ptr<descriptor_layout_cache> cache;
	std::unique_ptr<descriptor_allocator> alloc;
};

#endif // !VULKAN_DESCRIPTOR_BUILDER
