#include "vulkan_swapchain.h"

#include "vulkan_types.inl"
#include "vulkan_device.h"
#include "vulkan_image.h"

#include <iostream>

#include "core/renderer/renderer_defines.h"

//number of images

b8 vulkan_swapchain_create(VulkanContext* context, const SwapchainDesc* desc, VulkanSwapchain** pp_out_swapchain)
{
    assert(context);
    assert(desc);
    assert(pp_out_swapchain);

    uint32_t width = desc->width;
    uint32_t height = desc->height;
    uint32_t number_of_images = desc->imageCount;

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

    u32 number_of_images = (surface_capabilites.minImageCount + 1 > ) ;

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

    width = extent.width;
    height = extent.height;
    context->framebuffer_width = width;
    context->framebuffer_height = height;

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

    swapchain->image_count = 0;
    vkGetSwapchainImagesKHR(context->device_context.handle, swapchain->handle, &swapchain->image_count, 0);

    if (swapchain->image_count != 0)
    {
	    swapchain->images.resize(swapchain->image_count);
		swapchain->image_views.resize(swapchain->image_count);
    }
      
    vkGetSwapchainImagesKHR(context->device_context.handle, swapchain->handle, &swapchain->image_count, swapchain->images.data());
    

    for (u32 i = 0; i < swapchain->image_count; ++i) {

        VkImageViewCreateInfo create_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        create_info.image = swapchain->images.at(i);
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain->image_format.format;
        create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.layerCount = 1;
        create_info.subresourceRange.levelCount = 1;

        VK_CHECK(vkCreateImageView(context->device_context.handle, &create_info, context->allocator, &swapchain->image_views.at(i)));
    }

    if (!vulkan_device_detect_depth_format(&context->device_context)) {
        std::cout << "depth format not detected" << std::endl;
        return false;
    }

    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        context->device_context.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU ONLY
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depth_attachment
    );

    std::cout << "swapchain created" << std::endl;

	return true;
}

b8 vulkan_swapchain_destroy(VulkanContext* context, VulkanSwapchain* swapchain)
{
    if (context->render_fences.at(context->current_frame) != VK_NULL_HANDLE)
        VK_CHECK(vkWaitForFences(context->device_context.handle, 1, &context->render_fences.at(context->current_frame), true, UINT64_MAX));

    vulkan_image_destroy(context, &swapchain->depth_attachment);
   
    for (u32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(context->device_context.handle, swapchain->image_views.at(i), context->allocator);
    }

    vkDestroySwapchainKHR(context->device_context.handle, swapchain->handle, context->allocator);

    swapchain->handle = nullptr;

	return true;
}

b8 vulkan_swapchain_recreate(VulkanContext* context, i32 width, i32 height)
{
    VulkanSwapchain out_swapchain{};
	
    if (!vulkan_swapchain_create(context, width, height, &out_swapchain))
        return false;


	vulkan_swapchain_destroy(context, &context->swapchain);

	if(context->swapchain.handle != nullptr) {
        vkDestroySwapchainKHR(context->device_context.handle, context->swapchain.handle, context->allocator);
    }

    context->swapchain = out_swapchain;

	return true;
}

void vulkan_swapchain_get_support_info(VulkanContext* context, VulkanSwapchainSupportInfo* out_support_info)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device_context.physical_device, context->surface, &out_support_info->surface_capabilites);

    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface, &out_support_info->format_count, 0);
    out_support_info->surface_formats.resize(out_support_info->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface, &out_support_info->format_count, out_support_info->surface_formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device_context.physical_device, context->surface, &out_support_info->present_mode_count, 0);
    out_support_info->present_modes.resize(out_support_info->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device_context.physical_device, context->surface, &out_support_info->present_mode_count, out_support_info->present_modes.data());
}

b8 acquire_next_image_index_swapchain(
    VulkanContext* context,
    VulkanSwapchain* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* image_index)
{
    VkResult result = vkAcquireNextImageKHR(context->device_context.handle, swapchain->handle, timeout_ns, image_available_semaphore, fence, image_index);

    switch (result) {
    case VK_ERROR_OUT_OF_DATE_KHR:
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_width);
        return false;
    case VK_SUBOPTIMAL_KHR:
        break;
        // if the swapchain no longer matches the surface properties exactly,
        // -but can still be used for presentation
    }

    return true;
}

b8 present_image_swapchain(
    VulkanContext* context,
    VulkanSwapchain* swapchain,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 current_image_index
) 
{
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &current_image_index;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);

    switch (result) {
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
        return false;
    }

    return true;
}
