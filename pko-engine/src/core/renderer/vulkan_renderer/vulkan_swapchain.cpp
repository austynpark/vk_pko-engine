#include "vulkan_swapchain.h"

#include "vulkan_types.inl"

#include <iostream>

//number of images

b8 vulkan_swapchain_create(vulkan_context* context, i32 width, i32 height,vulkan_swapchain* swapchain)
{
    VkExtent2D extent{ width, height };

    vulkan_swapchain_get_support_info(context, &context->swapchain_support_info);

    // find surface format
    b8 is_found = false;

    for (u32 i = 0; i < context->swapchain_support_info.format_count; ++i) {

        VkSurfaceFormatKHR surface_format = context->swapchain_support_info.surface_formats[i];

        if ((surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) &&
            (surface_format.format == VkFormat::VK_FORMAT_B8G8R8A8_UNORM))
        {
            swapchain->image_format = surface_format;
            is_found = true;
            break;
        }
    }

    if (!is_found) {
        std::cout << "swapchain create: failed to find surface format\n";
        return false;
    }


    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context->swapchain_support_info.present_mode_count; ++i) {
        if (context->swapchain_support_info.present_modes.at(i) == VK_PRESENT_MODE_MAILBOX_KHR) {
            selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    
    swapchain->present_mode = selected_present_mode;

    VkSurfaceCapabilitiesKHR surface_capabilites = context->swapchain_support_info.surface_capabilites;

    u32 number_of_images = surface_capabilites.minImageCount + 1;

    if (surface_capabilites.maxImageCount > 0 &&
        number_of_images > surface_capabilites.maxImageCount) {
        number_of_images = surface_capabilites.maxImageCount;
    }

    if (surface_capabilites.currentExtent.width == UINT32_MAX) {
        if (extent.width < surface_capabilites.minImageExtent.width) {
            extent.width = surface_capabilites.minImageExtent.width;
        }
        else if (extent.width > surface_capabilites.maxImageExtent.width) {
            extent.width = surface_capabilites.maxImageExtent.width;
        }

        if (extent.height < surface_capabilites.minImageExtent.height) {
            extent.height = surface_capabilites.minImageExtent.height;
        }
        else if (extent.height > surface_capabilites.maxImageExtent.height) {
            extent.height = surface_capabilites.maxImageExtent.height;
        }
    }
    else {
        extent = surface_capabilites.currentExtent;
    }

	VkSwapchainCreateInfoKHR swapchain_create_info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = number_of_images;
    swapchain_create_info.imageFormat = swapchain->image_format.format;
    swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->device_context.graphics_family.index != context->device_context.present_family.index) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        u32 queue_family_indices[] = {
            context->device_context.graphics_family.index,
            context->device_context.transfer_family.index
        };
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 1;
        u32 queue_family_indices[] = {
            context->device_context.graphics_family.index
        };
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }

    swapchain_create_info.preTransform = context->swapchain_support_info.surface_capabilites.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = selected_present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = context->swapchain.handle;

	VK_CHECK(vkCreateSwapchainKHR(context->device_context.handle, &swapchain_create_info, context->allocator, &swapchain->handle));

	return true;
}

b8 vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{


	return b8();
}

b8 vulkan_swapchain_recreate(vulkan_context* context, i32 width, i32 height)
{
    vulkan_swapchain out_swapchain{};

    if (!vulkan_swapchain_create(context, width, height, &out_swapchain))
        return false;

    vulkan_swapchain_destroy(context, &context->swapchain);

    context->swapchain = out_swapchain;

	return true;
}

void vulkan_swapchain_get_support_info(vulkan_context* context, vulkan_swapchain_support_info* out_support_info)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device_context.physical_device, context->surface, &out_support_info->surface_capabilites);

    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface, &out_support_info->format_count, out_support_info->surface_formats.data());
    out_support_info->surface_formats.resize(out_support_info->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface, &out_support_info->format_count, 0);

    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device_context.physical_device, context->surface, &out_support_info->present_mode_count, 0);
    out_support_info->present_modes.resize(out_support_info->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device_context.physical_device, context->surface, &out_support_info->present_mode_count, out_support_info->present_modes.data());
}
