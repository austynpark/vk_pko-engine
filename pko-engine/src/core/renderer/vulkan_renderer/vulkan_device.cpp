#include "vulkan_device.h"

#include "vulkan_types.inl"

#include <iostream>

struct queue_family_info
{
    u32 graphics_queue_family_index;
    u32 present_queue_family_index;
    u32 transfer_queue_family_index;
    u32 compute_queue_family_index;
};

struct device_requirements
{
    b8 use_graphics;
    b8 use_present;
    b8 use_transfer;
    b8 use_compute;
    b8 use_discrete_gpu;

    std::vector<const char*> extensions_name;
};

b8 pick_physical_device(RenderContext* context, device_requirements* requirements);
b8 check_physical_device_requirements(VkPhysicalDevice device, VkSurfaceKHR surface,
                                      device_requirements* requirements,
                                      queue_family_info* out_queue_family);

b8 vulkan_device_create(RenderContext* context, DeviceContext* device_context)
{
    device_requirements requirements{
        true,   // b8 use_graphics;
        true,   // b8 use_present;
        true,   // b8 use_compute;
        true,   // b8 use_transfer;
        false,  // b8 use_discrete_gpu;
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME}};

    if (!pick_physical_device(context, &requirements))
    {
        return false;
    }

    u32 queue_count = 1;

    b8 present_shares_graphics_queue = (context->device_context.graphics_family.index ==
                                        context->device_context.present_family.index);

    b8 transfer_shares_graphics_queue = (context->device_context.graphics_family.index ==
                                         context->device_context.transfer_family.index);

    if (context->device_context.compute_family.index != UINT32_MAX)
        ++queue_count;

    if (!present_shares_graphics_queue)
    {
        ++queue_count;
    }

    if (!transfer_shares_graphics_queue)
    {
        ++queue_count;
    }

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_count);
    std::vector<u32> queue_indices(queue_count);
    u32 queue_index_count = 0;
    queue_indices.at(queue_index_count++) = context->device_context.graphics_family.index;

    if (!present_shares_graphics_queue)
    {
        queue_indices.at(queue_index_count++) = context->device_context.present_family.index;
    }

    if (!transfer_shares_graphics_queue)
    {
        queue_indices.at(queue_index_count++) = context->device_context.transfer_family.index;
    }

    if (context->device_context.compute_family.index != UINT32_MAX)
    {
        queue_indices.at(queue_index_count) = context->device_context.compute_family.index;
    }

    for (u32 i = 0; i < queue_count; ++i)
    {
        queue_create_infos.at(i).sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos.at(i).flags = 0;
        queue_create_infos.at(i).queueFamilyIndex = queue_indices.at(i);
        queue_create_infos.at(i).queueCount = 1;

        f32 queue_priorities = 1;
        queue_create_infos.at(i).pQueuePriorities = &queue_priorities;
    }

    // VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT pipeline_cache_control_features = {};
    // pipeline_cache_control_features.sType =
    //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT;
    // pipeline_cache_control_features.pipelineCreationCacheControl = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};
    dynamicRenderingFeaturesKHR.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeaturesKHR.dynamicRendering = VK_TRUE;
    dynamicRenderingFeaturesKHR.pNext = NULL;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    descriptorIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.pNext = &dynamicRenderingFeaturesKHR;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &descriptorIndexingFeatures;

    // Fetch all features from physical device
    vkGetPhysicalDeviceFeatures2(context->device_context.physical_device, &deviceFeatures);
    assert(descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind);
    assert(descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind);
    assert(descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind);

    VkDeviceCreateInfo device_create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.pNext = &deviceFeatures;
    device_create_info.queueCreateInfoCount = queue_count;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = requirements.extensions_name.size();
    device_create_info.ppEnabledExtensionNames = requirements.extensions_name.data();
    // device_create_info.pEnabledFeatures;

    VK_CHECK(vkCreateDevice(device_context->physical_device, &device_create_info,
                            context->allocator, &device_context->handle));

    std::cout << "device successfully created" << std::endl;

    return true;
}

b8 vulkan_device_destroy(RenderContext* context, DeviceContext* device_context)
{
    vkDestroyDevice(device_context->handle, context->allocator);

    return false;
}

void vulkan_get_device_queue(DeviceContext* device_context)
{
    if (device_context->graphics_family.index != -1)
        vkGetDeviceQueue(device_context->handle, device_context->graphics_family.index, 0,
                         &device_context->graphics_queue);
    if (device_context->present_family.index != -1)
        vkGetDeviceQueue(device_context->handle, device_context->present_family.index, 0,
                         &device_context->present_queue);
    if (device_context->transfer_family.index != -1)
        vkGetDeviceQueue(device_context->handle, device_context->transfer_family.index, 0,
                         &device_context->transfer_queue);
    if (device_context->compute_family.index != -1)
        vkGetDeviceQueue(device_context->handle, device_context->compute_family.index, 0,
                         &device_context->compute_queue);

    std::cout << "device queue acquired" << std::endl;
}

b8 pick_physical_device(RenderContext* context, device_requirements* requirements)
{
    u32 physical_count = 0;

    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_count, 0));

    if (physical_count == 0)
    {
        std::cout << "physical device doesn't exists\n";
        return false;
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_count);
    VK_CHECK(
        vkEnumeratePhysicalDevices(context->instance, &physical_count, physical_devices.data()));

    queue_family_info out_queue_family_info{UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};

    for (u32 i = 0; i < physical_count; ++i)
    {
        if (check_physical_device_requirements(physical_devices.at(i), context->surface,
                                               requirements, &out_queue_family_info))
        {
            context->device_context.physical_device = physical_devices.at(i);

            context->device_context.graphics_family.index =
                out_queue_family_info.graphics_queue_family_index;
            context->device_context.present_family.index =
                out_queue_family_info.present_queue_family_index;
            context->device_context.transfer_family.index =
                out_queue_family_info.transfer_queue_family_index;
            context->device_context.compute_family.index =
                out_queue_family_info.compute_queue_family_index;

            vkGetPhysicalDeviceFeatures(context->device_context.physical_device,
                                        &context->device_context.features);
            vkGetPhysicalDeviceProperties(context->device_context.physical_device,
                                          &context->device_context.properties);

            break;
        }
    }

    std::cout << "physical device found" << std::endl;

    return true;
}

b8 check_physical_device_requirements(VkPhysicalDevice device, VkSurfaceKHR surface,
                                      device_requirements* requirements,
                                      queue_family_info* out_queue_family)
{
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceProperties device_properties;

    vkGetPhysicalDeviceFeatures(device, &device_features);
    vkGetPhysicalDeviceProperties(device, &device_properties);

    if (requirements->use_discrete_gpu)
    {
        if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return false;
    }

    u32 available_extension_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));

    if (available_extension_count == 0)
    {
        std::cout << "can't find device extension properties\n";
        return false;
    }

    std::vector<VkExtensionProperties> extension_properties(available_extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count,
                                                  extension_properties.data()));

    u32 required_extension_count = requirements->extensions_name.size();

    b8 found = false;

    for (u32 i = 0; i < required_extension_count; ++i)
    {
        found = false;

        for (u32 k = 0; k < available_extension_count; ++k)
        {
            if (strcmp(extension_properties.at(k).extensionName,
                       requirements->extensions_name[i]) == 0)
            {
                std::cout << requirements->extensions_name[i] << " is missing" << std::endl;
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, 0);

    if (queue_families_count == 0)
    {
        std::cout << "queue family doesn't exists\n";
        return false;
    }

    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());

    u32 graphics_queue_family_index = UINT32_MAX;
    u32 present_queue_family_index = UINT32_MAX;
    u32 transfer_queue_family_index = UINT32_MAX;
    u32 compute_queue_family_index = UINT32_MAX;

    for (u32 i = 0; i < queue_families_count; ++i)
    {
        if (requirements->use_graphics &&
            (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX))
        {
            if ((queue_families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                graphics_queue_family_index = i;

            VkBool32 present_supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_supported);

            if (present_supported && requirements->use_present)
            {
                present_queue_family_index = i;
            }
        }

        if (requirements->use_transfer)
        {
            if ((queue_families.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
                transfer_queue_family_index = i;
        }

        if (requirements->use_compute && (graphics_queue_family_index != i))
        {
            if ((queue_families.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
                compute_queue_family_index = i;
        }
    }

    if (requirements->use_graphics)
    {
        if (graphics_queue_family_index == UINT32_MAX)
        {
            return false;
        }

        out_queue_family->graphics_queue_family_index = graphics_queue_family_index;
    }

    if (requirements->use_present)
    {
        if (present_queue_family_index == UINT32_MAX)
        {
            return false;
        }

        out_queue_family->present_queue_family_index = present_queue_family_index;
    }

    if (requirements->use_transfer)
    {
        if (transfer_queue_family_index == UINT32_MAX)
        {
            return false;
        }

        out_queue_family->transfer_queue_family_index = transfer_queue_family_index;
    }

    if (requirements->use_compute)
    {
        if (compute_queue_family_index == UINT32_MAX)
        {
            return false;
        }

        out_queue_family->compute_queue_family_index = compute_queue_family_index;
    }

    return true;
}

b8 vulkan_device_detect_depth_format(DeviceContext* device_context)
{
    const u64 candidate_count = 3;
    VkFormat candidates[candidate_count] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                            VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(device_context->physical_device, candidates[i],
                                            &properties);

        if ((flags & properties.linearTilingFeatures) == flags)
        {
            device_context->depth_format = candidates[i];
            return true;
        }
        else if ((flags & properties.optimalTilingFeatures) == flags)
        {
            device_context->depth_format = candidates[i];
            return true;
        }
    }

    return false;
}
