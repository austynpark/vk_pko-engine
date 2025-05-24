#include "vulkan_image.h"

#include "vendor/mmgr/mmgr.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_types.inl"

#define STB_IMAGE_IMPLEMENTATION
#include <iostream>

#include "stb_image.h"

b8 vulkan_rendertarget_create(RenderContext* context, RenderTargetDesc* desc,
                              RenderTarget** out_render_target)
{
    assert(context);
    assert(desc);
    assert(out_render_target);

    RenderTarget* render_target = (RenderTarget*)alloc_aligned_memory(
        sizeof(RenderTarget) + ((desc->descriptor_type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                                    ? sizeof(VkImageView) * desc->mip_levels
                                    : 0),
        16);
    render_target->array_descriptors = (VkImageView*)(render_target + 1);

    const bool isDepth = is_image_format_depth_only(desc->vulkan_format) ||
                         is_image_format_depth_stencil(desc->vulkan_format);
    assert(render_target);

    TextureDesc textureDesc = {};
    textureDesc.width = desc->width;
    textureDesc.height = desc->height;
    textureDesc.mip_levels = desc->mip_levels;
    textureDesc.sample_count = 1;
    textureDesc.start_state = !isDepth ? RESOURCE_STATE_RENDER_TARGET : RESOURCE_STATE_DEPTH_WRITE;
    textureDesc.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureDesc.vulkan_format = desc->vulkan_format;
    textureDesc.clear_value = desc->clear_value;
    textureDesc.type = VkDescriptorType(desc->descriptor_type | VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    textureDesc.native_handle = desc->native_handle;

    vulkan_texture_create(context, &textureDesc, &render_target->texture);

    render_target->width = desc->width;
    render_target->height = desc->height;
    render_target->mip_levels = desc->mip_levels;
    render_target->vulkan_format = desc->vulkan_format;
    render_target->descriptors = desc->descriptor_type;
    render_target->clear_value = desc->clear_value;

    // TODO(FUTURE)
    render_target->array_size = 1;
    render_target->depth = 1;
    render_target->sample_count = VK_SAMPLE_COUNT_1_BIT;
    render_target->sample_quality = 0;

    // srv
    VkImageViewCreateInfo imgViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imgViewCreateInfo.flags = 0;
    imgViewCreateInfo.image = render_target->texture->image;
    imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo.format = render_target->vulkan_format;
    imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    imgViewCreateInfo.subresourceRange.aspectMask =
        format_to_vulkan_image_aspect(render_target->vulkan_format);
    imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imgViewCreateInfo.subresourceRange.levelCount = 1;
    imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device_context.handle, &imgViewCreateInfo,
                               context->allocator, &render_target->descriptor));

    if (render_target->mip_levels > 1)
    {
        for (u32 i = 0; i < render_target->mip_levels; ++i)
        {
            imgViewCreateInfo.subresourceRange.baseMipLevel = i;
            VK_CHECK(vkCreateImageView(context->device_context.handle, &imgViewCreateInfo,
                                       context->allocator, &render_target->array_descriptors[i]));
        }
    }

    Command oneTimeSubmit;
    vulkan_command_pool_create(context, &oneTimeSubmit, QUEUE_TYPE_GRAPHICS);
    vulkan_command_buffer_allocate(context, &oneTimeSubmit, true);
    vulkan_command_buffer_begin(&oneTimeSubmit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    TextureBarrier textureBarrier;
    textureBarrier.current_state = RESOURCE_STATE_UNDEFINED;
    textureBarrier.new_state = desc->start_state;
    textureBarrier.texture = render_target->texture;

    vulkan_command_resource_barrier(&oneTimeSubmit, NULL, 0, &textureBarrier, 1, NULL, 0);

    vulkan_command_buffer_end(&oneTimeSubmit);

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &oneTimeSubmit.buffer;

    VK_CHECK(
        vkQueueSubmit(context->device_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(context->device_context.graphics_queue);
    return true;
}

void vulkan_rendertarget_destroy(RenderContext* context, RenderTarget* render_target)
{
    vulkan_texture_destroy(context, render_target->texture);

    if (render_target->descriptor != VK_NULL_HANDLE)
    {
        vkDestroyImageView(context->device_context.handle, render_target->descriptor,
                           context->allocator);
    }

    if (render_target->array_descriptors)
    {
        for (u32 i = 0; i < render_target->mip_levels; ++i)
        {
            vkDestroyImageView(context->device_context.handle, render_target->array_descriptors[i],
                               context->allocator);
        }
    }

    SAFE_FREE(render_target);
}

b8 vulkan_swapchain_create(RenderContext* context, const SwapchainDesc* desc,
                           VulkanSwapchain** ppSwapchain)
{
    assert(context);
    assert(desc);
    assert(ppSwapchain);

    VulkanSwapchain* swapchain = (VulkanSwapchain*)alloc_aligned_memory(
        sizeof(VulkanSwapchain) + sizeof(RenderTarget*) * desc->image_count + sizeof(SwapchainDesc),
        16);
    swapchain->render_targets = (RenderTarget**)(swapchain + 1);
    swapchain->desc = (SwapchainDesc*)(swapchain->render_targets + desc->image_count);
    assert(swapchain);

    uint32_t width = desc->width;
    uint32_t height = desc->height;
    uint32_t number_of_images = desc->image_count;

    VkExtent2D extent{width, height};

    vulkan_swapchain_get_support_info(context, &context->swapchain_support_info);

    // find surface format
    b8 is_found = false;

    for (u32 i = 0; i < context->swapchain_support_info.format_count; ++i)
    {
        VkSurfaceFormatKHR surface_format = context->swapchain_support_info.surface_formats[i];

        if ((surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) &&
            (surface_format.format == VkFormat::VK_FORMAT_B8G8R8A8_SRGB))
        {
            swapchain->surface_format = surface_format;
            is_found = true;
            break;
        }
    }

    if (!is_found)
    {
        std::cout << "swapchain create: failed to find surface format\n";
        return false;
    }

    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context->swapchain_support_info.present_mode_count; ++i)
    {
        if (context->swapchain_support_info.present_modes.at(i) == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    VkSurfaceCapabilitiesKHR surface_capabilites =
        context->swapchain_support_info.surface_capabilites;

    assert(surface_capabilites.minImageCount <= number_of_images);

    if (surface_capabilites.currentExtent.width == UINT32_MAX)
    {
        if (extent.width < surface_capabilites.minImageExtent.width)
        {
            extent.width = surface_capabilites.minImageExtent.width;
        }
        else if (extent.width > surface_capabilites.maxImageExtent.width)
        {
            extent.width = surface_capabilites.maxImageExtent.width;
        }

        if (extent.height < surface_capabilites.minImageExtent.height)
        {
            extent.height = surface_capabilites.minImageExtent.height;
        }
        else if (extent.height > surface_capabilites.maxImageExtent.height)
        {
            extent.height = surface_capabilites.maxImageExtent.height;
        }
    }
    else
    {
        extent = surface_capabilites.currentExtent;
    }

    width = extent.width;
    height = extent.height;
    context->framebuffer_width = width;
    context->framebuffer_height = height;

    VkSwapchainCreateInfoKHR swapchain_create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = number_of_images;
    swapchain_create_info.imageFormat = swapchain->surface_format.format;
    swapchain_create_info.imageColorSpace = swapchain->surface_format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->device_context.graphics_family.index !=
        context->device_context.present_family.index)
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        u32 queue_family_indices[] = {context->device_context.graphics_family.index,
                                      context->device_context.transfer_family.index};
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 1;
        u32 queue_family_indices[] = {context->device_context.graphics_family.index};
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }

    swapchain_create_info.preTransform =
        context->swapchain_support_info.surface_capabilites.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = selected_present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device_context.handle, &swapchain_create_info,
                                  context->allocator, &swapchain->handle));

    swapchain->image_count = 0;
    vkGetSwapchainImagesKHR(context->device_context.handle, swapchain->handle,
                            &swapchain->image_count, 0);

    VkImage* images = (VkImage*)malloc(sizeof(VkImage) * swapchain->image_count);

    if (swapchain->image_count != 0)
    {
        vkGetSwapchainImagesKHR(context->device_context.handle, swapchain->handle,
                                &swapchain->image_count, images);
    }

    RenderTargetDesc renderTargetDesc = {};
    renderTargetDesc.clear_value = {{0.f, 0.f, 0.f, 0.f}};
    renderTargetDesc.width = desc->width;
    renderTargetDesc.height = desc->height;
    renderTargetDesc.vulkan_format = swapchain->surface_format.format;
    renderTargetDesc.mip_levels = 1;
    renderTargetDesc.sample_count = 1;
    renderTargetDesc.start_state = RESOURCE_STATE_PRESENT;

    for (uint32_t rt = 0; rt < number_of_images; ++rt)
    {
        renderTargetDesc.native_handle = (void*)images[rt];  // VkImage
        vulkan_rendertarget_create(context, &renderTargetDesc, &swapchain->render_targets[rt]);
    }

    if (images)
    {
        free(images);
    }

    // if (!vulkan_device_detect_depth_format(&context->device_context)) {
    //     std::cout << "depth format not detected" << std::endl;
    //     return false;
    // }

    // vulkan_image_create(
    //     context,
    //     VK_IMAGE_TYPE_2D,
    //     width,
    //     height,
    //     context->device_context.depth_format,
    //     VK_IMAGE_TILING_OPTIMAL,
    //     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU ONLY
    //     true,
    //     VK_IMAGE_ASPECT_DEPTH_BIT,
    //     &swapchain->depth_attachment
    //);

    std::cout << "swapchain created" << std::endl;

    *ppSwapchain = swapchain;

    return true;
}

b8 vulkan_swapchain_destroy(RenderContext* context, VulkanSwapchain* swapchain)
{
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        vulkan_rendertarget_destroy(context, swapchain->render_targets[i]);
    }

    vkDestroySwapchainKHR(context->device_context.handle, swapchain->handle, context->allocator);

    swapchain->handle = NULL;
    SAFE_FREE(swapchain);

    return true;
}

b8 vulkan_swapchain_recreate(RenderContext* context, VulkanSwapchain** ppSwapchain, i32 width,
                             i32 height)
{
    assert(ppSwapchain);

    VulkanSwapchain* swapchain = *ppSwapchain;
    swapchain->desc->width = width;
    swapchain->desc->height = height;

    if (!vulkan_swapchain_create(context, swapchain->desc, &swapchain))
        return false;

    // vulkan_swapchain_destroy(context, &context->swapchain);

    // if(context->swapchain.handle != nullptr) {
    //     vkDestroySwapchainKHR(context->device_context.handle,
    //     context->swapchain.handle, context->allocator);
    // }

    // context->swapchain = out_swapchain;

    return true;
}

void vulkan_swapchain_get_support_info(RenderContext* context,
                                       VulkanSwapchainSupportInfo* out_support_info)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device_context.physical_device,
                                              context->surface,
                                              &out_support_info->surface_capabilites);

    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface,
                                         &out_support_info->format_count, 0);
    out_support_info->surface_formats.resize(out_support_info->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device_context.physical_device, context->surface,
                                         &out_support_info->format_count,
                                         out_support_info->surface_formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device_context.physical_device,
                                              context->surface,
                                              &out_support_info->present_mode_count, 0);
    out_support_info->present_modes.resize(out_support_info->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        context->device_context.physical_device, context->surface,
        &out_support_info->present_mode_count, out_support_info->present_modes.data());
}

b8 acquire_next_image_index_swapchain(RenderContext* context, VulkanSwapchain* swapchain,
                                      u64 timeout_ns, VkSemaphore image_available_semaphore,
                                      VkFence fence, u32* image_index)
{
    VkResult result =
        vkAcquireNextImageKHR(context->device_context.handle, swapchain->handle, timeout_ns,
                              image_available_semaphore, fence, image_index);

    switch (result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
            // TODO
            // vulkan_swapchain_recreate(context, context->framebuffer_width,
            // context->framebuffer_width);
            return false;
        case VK_SUBOPTIMAL_KHR:
            break;
            // if the swapchain no longer matches the surface properties exactly,
            // -but can still be used for presentation
    }

    return true;
}

b8 present_image_swapchain(RenderContext* context, VulkanSwapchain* swapchain,
                           VkQueue present_queue, VkSemaphore render_complete_semaphore,
                           u32 current_image_index)
{
    VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &current_image_index;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);

    switch (result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            return false;
    }

    return true;
}

void vulkan_framebuffer_create(RenderContext* context, VulkanRenderpass* renderpass,
                               u32 attachment_count, VkImageView* image_view,
                               VkFramebuffer* out_framebuffer)
{
    VkFramebufferCreateInfo create_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    create_info.renderPass = renderpass->handle;
    create_info.attachmentCount = attachment_count;
    create_info.pAttachments = image_view;
    create_info.width = context->framebuffer_width;
    create_info.height = context->framebuffer_height;
    create_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(context->device_context.handle, &create_info, context->allocator,
                                 out_framebuffer));
}

void vulkan_framebuffer_destroy(RenderContext* context, VkFramebuffer* framebuffer)
{
    vkDestroyFramebuffer(context->device_context.handle, *framebuffer, context->allocator);
    framebuffer = 0;
}

bool is_image_format_depth_only(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            return true;
        default:
            return false;
    }
}

bool is_image_format_depth_stencil(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return true;
        default:
            return false;
    }
}

VkSampleCountFlagBits to_vulkan_sample_count(u32 sampleCount)
{
    switch (sampleCount)
    {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        case 64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            assert(false);
    }

    return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
}

VkImageUsageFlags descriptor_type_to_vulkan_image_usage(VkDescriptorType type)
{
    VkImageUsageFlags flags = 0;

    if (type & VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if (type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;

    return flags;
}

VkImageUsageFlags resource_state_to_vulkan_image_usage(ResourceState state)
{
    VkImageUsageFlags flags = 0;

    if (state & RESOURCE_STATE_DEPTH_WRITE)
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (state & RESOURCE_STATE_RENDER_TARGET)
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (state & RESOURCE_STATE_COPY_DEST)
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (state & RESOURCE_STATE_COPY_SOURCE)
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    return flags;
}

VkImageAspectFlags format_to_vulkan_image_aspect(VkFormat format)
{
    VkImageAspectFlags flags = 0;

    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            flags = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case VK_FORMAT_S8_UINT:
            flags = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return flags;
}

VkImageLayout resource_state_to_vulkan_image_layout(ResourceState state)
{
    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;

    switch (state)
    {
        case RESOURCE_STATE_COMMON:
            result = VK_IMAGE_LAYOUT_GENERAL;
            break;
        case RESOURCE_STATE_COPY_DEST:
            result = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case RESOURCE_STATE_COPY_SOURCE:
            result = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;
        case RESOURCE_STATE_DEPTH_WRITE:
            result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        case RESOURCE_STATE_DEPTH_READ:
            result = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_PRESENT:
            result = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case RESOURCE_STATE_RENDER_TARGET:
            result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        default:
            result = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
    }

    return result;
}

void vulkan_texture_create(RenderContext* context, TextureDesc* desc, Texture** ptexture)
{
    assert(context);
    assert(desc);
    assert(ptexture);

    Texture* texture = (Texture*)alloc_aligned_memory(
        sizeof(Texture) + desc->type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            ? sizeof(VkImageView) * desc->mip_levels
            : 0,
        16);
    memset(texture, 0,
           sizeof(sizeof(Texture) + desc->type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                      ? sizeof(VkImageView) * desc->mip_levels
                      : 0));

    if (desc->type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        texture->uav_descriptors = (VkImageView*)(texture + 1);

    if (desc->native_handle == NULL)
    {
        texture->owns_image = true;
    }
    else
    {
        texture->owns_image = false;
        texture->image = (VkImage)desc->native_handle;
    }

    texture->width = desc->width;
    texture->height = desc->height;
    texture->vulkan_format = desc->vulkan_format;
    texture->sample_count = desc->sample_count;
    texture->mip_levels = desc->mip_levels;

    if (texture->image == VK_NULL_HANDLE)
    {
        VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imgCreateInfo.pNext = NULL;
        imgCreateInfo.flags = 0;
        imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imgCreateInfo.format = desc->vulkan_format;
        imgCreateInfo.extent.width = desc->width;
        imgCreateInfo.extent.height = desc->height;
        imgCreateInfo.extent.depth = 1;
        imgCreateInfo.mipLevels = desc->mip_levels;
        imgCreateInfo.arrayLayers = 1;
        imgCreateInfo.samples = to_vulkan_sample_count(desc->sample_count);
        imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgCreateInfo.usage = descriptor_type_to_vulkan_image_usage(desc->type) |
                              resource_state_to_vulkan_image_usage(desc->start_state);
        imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imgCreateInfo.queueFamilyIndexCount = 0;
        imgCreateInfo.pQueueFamilyIndices = nullptr;
        imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VK_CHECK(vmaCreateImage(context->vma_allocator, &imgCreateInfo, &allocCreateInfo,
                                &texture->image, &texture->pAlloc, nullptr));
    }

    // SRV
    VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = texture->image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = desc->vulkan_format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = desc->mip_levels;
    viewCreateInfo.subresourceRange.aspectMask = format_to_vulkan_image_aspect(desc->vulkan_format);
    texture->aspect_mask = format_to_vulkan_image_aspect(desc->vulkan_format);

    if (desc->type & VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
    {
        VK_CHECK(vkCreateImageView(context->device_context.handle, &viewCreateInfo,
                                   context->allocator, &texture->srv_descriptor));
    }

    if (desc->type & VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
    {
        viewCreateInfo.subresourceRange.levelCount = 1;
        for (u32 i = 0; i < desc->mip_levels; ++i)
        {
            viewCreateInfo.subresourceRange.baseMipLevel = i;
            VK_CHECK(vkCreateImageView(context->device_context.handle, &viewCreateInfo,
                                       context->allocator, &texture->uav_descriptors[i]));
        }
    }

    *ptexture = texture;
}

void vulkan_texture_destroy(RenderContext* context, Texture* texture)
{
    assert(context);
    assert(texture);

    if (texture->srv_descriptor != VK_NULL_HANDLE)
    {
        vkDestroyImageView(context->device_context.handle, texture->srv_descriptor,
                           context->allocator);
    }

    if (texture->uav_descriptors)
    {
        for (u32 i = 0; i < texture->mip_levels; ++i)
        {
            vkDestroyImageView(context->device_context.handle, texture->uav_descriptors[i],
                               context->allocator);
        }
    }

    if (texture->owns_image && (texture->image != VK_NULL_HANDLE))
    {
        vmaDestroyImage(context->vma_allocator, texture->image, texture->pAlloc);
    }
}

b8 load_image_from_file(RenderContext* context, const char* file, Texture* texture)
{
    i32 tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(file, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels)
    {
        std::cout << "failed to load texture file " << file << std::endl;
        return false;
    }

    void* pixel_ptr = pixels;

    Buffer staging_buffer;
    u32 staging_buffer_size = tex_width * tex_height * 4;

    vulkan_buffer_create(
        context, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        &staging_buffer);
    vulkan_buffer_upload(context, &staging_buffer, pixel_ptr, staging_buffer_size);
    stbi_image_free(pixels);

    // vulkan_image_create(
    //     context, VK_IMAGE_TYPE_2D,
    //     tex_width, tex_height,
    //     VK_FORMAT_R8G8B8A8_SRGB,
    //     VK_IMAGE_TILING_OPTIMAL,
    //     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    //     VMA_MEMORY_USAGE_GPU_ONLY,
    //     true,
    //     VK_IMAGE_ASPECT_COLOR_BIT,
    //     out_image
    //);

    // create one time submit command buffer for image copy from staging buffer
    Command one_time_submit;

    vulkan_command_pool_create(context, &one_time_submit, QUEUE_TYPE_TRANSFER);
    vulkan_command_buffer_allocate(context, &one_time_submit, true);

    vulkan_command_buffer_begin(&one_time_submit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // vulkan_image_layout_transition(
    //     out_image,
    //     &one_time_submit,
    //     VK_IMAGE_LAYOUT_UNDEFINED,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     0,
    //     VK_ACCESS_TRANSFER_WRITE_BIT,
    //     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //     VK_PIPELINE_STAGE_TRANSFER_BIT
    //);

    VkBufferImageCopy copy_region = {};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;

    // TODO: mipmap
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageExtent = {texture->width, texture->height, 1};

    // copy the buffer into the image
    vkCmdCopyBufferToImage(one_time_submit.buffer, staging_buffer.handle, texture->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    // vulkan_image_layout_transition(
    //     out_image,
    //     &one_time_submit,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //     VK_ACCESS_TRANSFER_WRITE_BIT,
    //     VK_ACCESS_SHADER_READ_BIT,
    //     VK_PIPELINE_STAGE_TRANSFER_BIT,
    //     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    //);

    vulkan_command_buffer_end(&one_time_submit);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &one_time_submit.buffer;

    VK_CHECK(
        vkQueueSubmit(context->device_context.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(context->device_context.transfer_queue);
    vulkan_command_pool_destroy(context, &one_time_submit);

    vulkan_buffer_destroy(context, &staging_buffer);

    return true;
}