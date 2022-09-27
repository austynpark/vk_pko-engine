#include "vulkan_types.inl"

#include <algorithm>

void descriptor_allocator::reset_pools()
{
	for (auto pool : used_pools) {
		vkResetDescriptorPool(device, pool, 0);
		free_pools.push_back(pool);
	}
	
	used_pools.clear();
	current_pool = VK_NULL_HANDLE;
}

bool descriptor_allocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
	if (current_pool == VK_NULL_HANDLE) {
		current_pool = grab_pool();
		used_pools.push_back(current_pool);
	}

	VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc_info.pNext = nullptr;
	alloc_info.pSetLayouts = &layout;
	alloc_info.descriptorPool = current_pool;
	alloc_info.descriptorSetCount = 1;

	VkResult result = vkAllocateDescriptorSets(device, &alloc_info, set);
	bool need_reallocate = false;

	switch (result) {
	case VK_SUCCESS:
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		// reallocate pool
		need_reallocate = true;
		break;
	default:
		return false;
	}

	if (need_reallocate) {
		current_pool = grab_pool();
		used_pools.push_back(current_pool);

		alloc_info.descriptorPool = current_pool;
		result = vkAllocateDescriptorSets(device, &alloc_info, set);
		
		if (result == VK_SUCCESS)
			return true;
	}

	return false;
}

void descriptor_allocator::init(VkDevice new_device)
{
	device = new_device;
}

void descriptor_allocator::cleanup()
{
	for (auto pool : free_pools) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

	for (auto pool : used_pools) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

}

VkDescriptorPool descriptor_allocator::grab_pool()
{
	// there are reusable pools avail
	if (free_pools.size() > 0) {
		// grab pool from the back of the vector and remove it from there
		VkDescriptorPool pool = free_pools.back();
		free_pools.pop_back();
		return pool;
	}

	// no pools avail, so create new one
	return descriptor_sizes.create_pool(device, 1000, 0);
}

// count: multiplier of each VkDescriptorType & max number of descriptor sets that can be allocated from the pool
VkDescriptorPool descriptor_allocator::pool_sizes::create_pool(VkDevice device, i32 count, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(this->sizes.size());

	for (auto sz : this->sizes) {
		sizes.push_back({ sz.first, (u32)(sz.second * count) });
	}

	VkDescriptorPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	create_info.flags = flags;
	create_info.maxSets = count;
	create_info.poolSizeCount = sizes.size();
	create_info.pPoolSizes = sizes.data();

	VkDescriptorPool pool;
	vkCreateDescriptorPool(device, &create_info, nullptr, &pool);

	return pool;
}

void descriptor_layout_cache::init(VkDevice new_device)
{
	device = new_device;
}

void descriptor_layout_cache::cleanup()
{
	for (auto cache : layout_cache) {
		vkDestroyDescriptorSetLayout(device, cache.second, nullptr);
	}

	layout_cache.clear();
}

VkDescriptorSetLayout descriptor_layout_cache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info)
{
	descriptor_layout_info layout_info;
	layout_info.bindings.resize(info->bindingCount);
	b8 is_sorted = true;
	i32 last_binding = -1;

	for (i32 i = 0; i < info->bindingCount; ++i) {
		layout_info.bindings[i] = info->pBindings[i];

		if (info->pBindings[i].binding > last_binding) {
			last_binding = info->pBindings[i].binding;
		}
		else {
			is_sorted = false;
		}
	}

	// sort the bindings if they aren't in order
	if (!is_sorted) {
		std::sort(layout_info.bindings.begin(), layout_info.bindings.end(),
			[](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
			{
				return lhs.binding < rhs.binding;
			});
	}



	if (layout_cache.find(layout_info) != layout_cache.end()) {
		return layout_cache[layout_info];
	}


	// create layout and add to cache
	VkDescriptorSetLayout layout;
	VK_CHECK(vkCreateDescriptorSetLayout(device, info, nullptr, &layout));

	layout_cache[layout_info] = layout;
	return layout;
}

bool descriptor_layout_cache::descriptor_layout_info::operator==(const descriptor_layout_info& other) const
{
	if (other.bindings.size() != bindings.size()) {
		return false;
	}
	else {
		i32 bindings_size = bindings.size();
		for (i32 i = 0; i < bindings_size; ++i) {
			if (other.bindings[i].binding != bindings[i].binding)
				return false;
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
				return false;
			if (other.bindings[i].descriptorType != bindings[i].descriptorType)
				return false;
			if (other.bindings[i].stageFlags != bindings[i].stageFlags)
				return false;
		}
	}

	return true;
}

size_t descriptor_layout_cache::descriptor_layout_info::hash() const
{
	size_t result = std::hash<size_t>()(bindings.size());

	for (const VkDescriptorSetLayoutBinding& b : bindings) {
		size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;
		result ^= std::hash<size_t>()(binding_hash);
	}

	return result;
}
