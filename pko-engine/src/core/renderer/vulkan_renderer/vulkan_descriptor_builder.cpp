#include "vulkan_descriptor_builder.h"

void descriptor_builder::begin(VkDevice device)
{
	alloc = new DescriptorAllocator();
    cache = new DescriptorLayoutCache();

    cache->init(device);
    alloc->init(device);
}

void descriptor_builder::cleanup()
{
	delete cache;
	cache = nullptr;
	delete alloc;
	alloc = nullptr;
}

descriptor_builder& descriptor_builder::bind_buffer(uint32_t binding, VkDescriptorBufferInfo* buffer_info, VkDescriptorType type, VkShaderStageFlags stage_flags)
{
	//create the descriptor binding for the layout
	VkDescriptorSetLayoutBinding new_binding{};

	new_binding.descriptorCount = 1;
	new_binding.descriptorType = type;
	new_binding.pImmutableSamplers = nullptr;
	new_binding.stageFlags = stage_flags;
	new_binding.binding = binding;

	bindings.push_back(new_binding);

	//create the descriptor write
	VkWriteDescriptorSet new_write{};
	new_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	new_write.pNext = nullptr;

	new_write.descriptorCount = 1;
	new_write.descriptorType = type;
	new_write.pBufferInfo = buffer_info;
	new_write.dstBinding = binding;

	writes.push_back(new_write);
	return *this;
}

descriptor_builder& descriptor_builder::bind_image(uint32_t binding, VkDescriptorImageInfo* image_info, VkDescriptorType type, VkShaderStageFlags stage_flags)
{
	//create the descriptor binding for the layout
	VkDescriptorSetLayoutBinding new_binding{};

	new_binding.descriptorCount = 1;
	new_binding.descriptorType = type;
	new_binding.pImmutableSamplers = nullptr;
	new_binding.stageFlags = stage_flags;
	new_binding.binding = binding;

	bindings.push_back(new_binding);

	//create the descriptor write
	VkWriteDescriptorSet new_write{};
	new_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	new_write.pNext = nullptr;

	new_write.descriptorCount = 1;
	new_write.descriptorType = type;
	new_write.pImageInfo = image_info;
	new_write.dstBinding = binding;

	writes.push_back(new_write);
	return *this;
}

b8 descriptor_builder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
	//build layout first
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pNext = nullptr;

	layout_info.pBindings = bindings.data();
	layout_info.bindingCount = bindings.size();

	layout = cache->create_descriptor_layout(&layout_info);

	//allocate descriptor
	bool success = alloc->allocate(&set, layout);
	if (!success) { return false; };

	//write descriptor
	for (VkWriteDescriptorSet& w : writes) {
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(alloc->device, writes.size(), writes.data(), 0, nullptr);

	return true;
}

b8 descriptor_builder::build(VkDescriptorSet& set)
{
	VkDescriptorSetLayout layout;
    return build(set, layout);
;
}
