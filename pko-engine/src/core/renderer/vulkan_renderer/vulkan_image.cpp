#include "vulkan_image.h"

#include "vulkan_types.inl"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"

#include "vendor/mmgr/mmgr.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

b8 vulkan_rendertarget_create(VulkanContext* pContext, RenderTargetDesc* pRenderTargetDesc, RenderTarget** ppRenderTarget)
{
    assert(pContext);
    assert(pRenderTargetDesc);
    assert(ppRenderTarget);

    RenderTarget* pRenderTarget = (RenderTarget*)alloc_aligned_memory(sizeof(RenderTarget) + pRenderTargetDesc->mDescriptorType & DESCRIPTOR_TYPE_RW_TEXTURE ? sizeof(VkImageView) * pRenderTargetDesc->mMipLevels : 0, 16);
    pRenderTarget->pArrayDescriptors = (VkImageView*)(pRenderTarget + 1);

    const bool isDepth = is_image_format_depth_only(pRenderTargetDesc->mFormat) || is_image_format_depth_stencil(pRenderTargetDesc->mFormat);
    assert(pRenderTarget);

    TextureDesc textureDesc = {};
    textureDesc.mWidth = pRenderTargetDesc->mWidth;
    textureDesc.mHeight = pRenderTargetDesc->mHeight;
    textureDesc.mMipLevels = pRenderTargetDesc->mMipLevels;
    textureDesc.mSampleCount = 1;
    textureDesc.mStartState = !isDepth ? RESOURCE_STATE_RENDER_TARGET : RESOURCE_STATE_DEPTH_WRITE;
    textureDesc.mType = DESCRIPTOR_TYPE_TEXTURE;
    textureDesc.mFormat = pRenderTargetDesc->mFormat;
    textureDesc.mClearValue = pRenderTargetDesc->mClearValue;
    textureDesc.mType = pRenderTargetDesc->mDescriptorType | DESCRIPTOR_TYPE_TEXTURE;
    textureDesc.pNativeHandle = pRenderTargetDesc->pNativeHandle;

    vulkan_texture_create(pContext, &textureDesc, &pRenderTarget->pTexture);

    pRenderTarget->mWidth = pRenderTargetDesc->mWidth;
    pRenderTarget->mHeight = pRenderTargetDesc->mHeight;
    pRenderTarget->mMipLevels = pRenderTargetDesc->mMipLevels;
    pRenderTarget->mFormat = pRenderTargetDesc->mFormat;
    pRenderTarget->mDescriptors = pRenderTargetDesc->mDescriptorType;
    pRenderTarget->mClearValue = pRenderTargetDesc->mClearValue;

    //TODO(FUTURE)
    pRenderTarget->mArraySize = 1;
    pRenderTarget->mDepth = 1;
    pRenderTarget->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
    pRenderTarget->mSampleQuality = 0;

    // srv
    VkImageViewCreateInfo imgViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imgViewCreateInfo.flags = 0;
    imgViewCreateInfo.image = pRenderTarget->pTexture->pImage;
    imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo.format = pRenderTarget->mFormat;
    imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    imgViewCreateInfo.subresourceRange.aspectMask;
    imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imgViewCreateInfo.subresourceRange.levelCount = 1;
    imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(pContext->device_context.handle, &imgViewCreateInfo, pContext->allocator, &pRenderTarget->pDescriptor));

    if (pRenderTarget->mMipLevels > 1)
    {
        for (u32 i = 0; i < pRenderTarget->mMipLevels; ++i)
        {
            imgViewCreateInfo.subresourceRange.baseMipLevel = i;
            VK_CHECK(vkCreateImageView(pContext->device_context.handle, &imgViewCreateInfo, pContext->allocator, &pRenderTarget->pArrayDescriptors[i]));
        }
    }
    
    Command oneTimeSubmit;
    vulkan_command_pool_create(pContext, &oneTimeSubmit, QUEUE_TYPE_GRAPHICS);
    vulkan_command_buffer_allocate(pContext, &oneTimeSubmit, true);
    vulkan_command_buffer_begin(&oneTimeSubmit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    TextureBarrier textureBarrier;
    textureBarrier.mCurrentState = RESOURCE_STATE_UNDEFINED;
    textureBarrier.mNewState = pRenderTargetDesc->mStartState;
    vulkan_command_resource_barrier(&oneTimeSubmit, NULL, 0, &textureBarrier, 1, NULL, 0);

    vulkan_command_buffer_end(&oneTimeSubmit);

    return true;
}

void vulkan_rendertarget_destroy(VulkanContext* pContext, RenderTarget* pRenderTarget)
{
    vulkan_texture_destroy(pContext, pRenderTarget->pTexture);

    if (pRenderTarget->pDescriptor!= VK_NULL_HANDLE)
    {
        vkDestroyImageView(pContext->device_context.handle, pRenderTarget->pDescriptor, pContext->allocator);
    }

    if (pRenderTarget->pArrayDescriptors)
    {
        for (u32 i = 0; i < pRenderTarget->mMipLevels; ++i)
        {
            vkDestroyImageView(pContext->device_context.handle, pRenderTarget->pArrayDescriptors[i], pContext->allocator);
        }
    }

    SAFE_FREE(pRenderTarget);
}

b8 vulkan_swapchain_create(VulkanContext* pContext, const SwapchainDesc* pDesc, VulkanSwapchain** ppSwapchain)
{
    assert(pContext);
    assert(pDesc);
    assert(ppSwapchain);

    VulkanSwapchain* pSwapchain = (VulkanSwapchain*)alloc_aligned_memory(sizeof(VulkanSwapchain) + sizeof(RenderTarget*) * pDesc->mImageCount + sizeof(SwapchainDesc), 16);
    pSwapchain->ppRenderTargets = (RenderTarget**)(pSwapchain + 1);
    pSwapchain->pDesc = (SwapchainDesc*)(pSwapchain->ppRenderTargets + pDesc->mImageCount);
    assert(pSwapchain);

    uint32_t width = pDesc->mWidth;
    uint32_t height = pDesc->mHeight;
    uint32_t number_of_images = pDesc->mImageCount;

    VkExtent2D extent{ width, height };

    vulkan_swapchain_get_support_info(pContext, &pContext->swapchain_support_info);

    // find surface format
    b8 is_found = false;

    for (u32 i = 0; i < pContext->swapchain_support_info.format_count; ++i) {

        VkSurfaceFormatKHR surface_format = pContext->swapchain_support_info.surface_formats[i];

        if ((surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) &&
            (surface_format.format == VkFormat::VK_FORMAT_B8G8R8A8_SRGB))
        {
            pSwapchain->mSurfaceFormat = surface_format;
            is_found = true;
            break;
        }
    }

    if (!is_found) {
        std::cout << "swapchain create: failed to find surface format\n";
        return false;
    }

    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < pContext->swapchain_support_info.present_mode_count; ++i) {
        if (pContext->swapchain_support_info.present_modes.at(i) == VK_PRESENT_MODE_MAILBOX_KHR) {
            selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    VkSurfaceCapabilitiesKHR surface_capabilites = pContext->swapchain_support_info.surface_capabilites;

    assert(surface_capabilites.minImageCount > number_of_images);

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
    pContext->framebuffer_width = width;
    pContext->framebuffer_height = height;

	VkSwapchainCreateInfoKHR swapchain_create_info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_create_info.surface = pContext->surface;
    swapchain_create_info.minImageCount = number_of_images;
    swapchain_create_info.imageFormat = pSwapchain->mSurfaceFormat.format;
    swapchain_create_info.imageColorSpace = pSwapchain->mSurfaceFormat.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (pContext->device_context.graphics_family.index != pContext->device_context.present_family.index) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        u32 queue_family_indices[] = {
            pContext->device_context.graphics_family.index,
            pContext->device_context.transfer_family.index
        };
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 1;
        u32 queue_family_indices[] = {
            pContext->device_context.graphics_family.index
        };
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }

    swapchain_create_info.preTransform = pContext->swapchain_support_info.surface_capabilites.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = selected_present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

	VK_CHECK(vkCreateSwapchainKHR(pContext->device_context.handle, &swapchain_create_info, pContext->allocator, &pSwapchain->handle));

    pSwapchain->mImageCount = 0;
    vkGetSwapchainImagesKHR(pContext->device_context.handle, pSwapchain->handle, &pSwapchain->mImageCount, 0);

    VkImage* pImages = (VkImage*)malloc(sizeof(VkImage) * pSwapchain->mImageCount);

    if (pSwapchain->mImageCount != 0)
    {
        vkGetSwapchainImagesKHR(pContext->device_context.handle, pSwapchain->handle, &pSwapchain->mImageCount, pImages);
    }
        
    RenderTargetDesc renderTargetDesc = {};
    renderTargetDesc.mClearValue = { {0.f, 0.f, 0.f, 0.f} };
    renderTargetDesc.mWidth = pDesc->mWidth;
    renderTargetDesc.mHeight = pDesc->mHeight;
    renderTargetDesc.mFormat = pSwapchain->mSurfaceFormat.format;
    renderTargetDesc.mMipLevels = 1;
    renderTargetDesc.mSampleCount = 1;
    renderTargetDesc.mStartState = RESOURCE_STATE_PRESENT;

    for (uint32_t rt = 0; rt < number_of_images; ++rt)
    {
        renderTargetDesc.pNativeHandle = (void*)pImages[rt]; // VkImage
        vulkan_rendertarget_create(pContext, &renderTargetDesc, &pSwapchain->ppRenderTargets[rt]);
    }

    SAFE_FREE(pImages);

    //if (!vulkan_device_detect_depth_format(&pContext->device_context)) {
    //    std::cout << "depth format not detected" << std::endl;
    //    return false;
    //}

    //vulkan_image_create(
    //    pContext,
    //    VK_IMAGE_TYPE_2D,
    //    width,
    //    height,
    //    pContext->device_context.depth_format,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU ONLY
    //    true,
    //    VK_IMAGE_ASPECT_DEPTH_BIT,
    //    &swapchain->depth_attachment
    //);

    std::cout << "swapchain created" << std::endl;

	return true;
}

b8 vulkan_swapchain_destroy(VulkanContext* pContext, VulkanSwapchain* pSwapchain)
{
    for (u32 i = 0; i < pSwapchain->mImageCount; ++i)
    {
        vulkan_rendertarget_destroy(pContext, pSwapchain->ppRenderTargets[i]);
    }

    vkDestroySwapchainKHR(pContext->device_context.handle, pSwapchain->handle, pContext->allocator);

    pSwapchain->handle = NULL;
    SAFE_FREE(pSwapchain);

	return true;
}

b8 vulkan_swapchain_recreate(VulkanContext* pContext, VulkanSwapchain** ppSwapchain, i32 width, i32 height)
{
    assert(ppSwapchain);

    VulkanSwapchain* pSwapchain = *ppSwapchain;
    pSwapchain->pDesc->mWidth = width;
    pSwapchain->pDesc->mHeight = height;

    if (!vulkan_swapchain_create(pContext, pSwapchain->pDesc, &pSwapchain))
        return false;


    //vulkan_swapchain_destroy(pContext, &pContext->swapchain);

    //if(pContext->swapchain.handle != nullptr) {
    //    vkDestroySwapchainKHR(pContext->device_context.handle, pContext->swapchain.handle, pContext->allocator);
    //}

    //pContext->swapchain = out_swapchain;

    return true;
}

void vulkan_swapchain_get_support_info(VulkanContext* pContext, VulkanSwapchainSupportInfo* out_support_info)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pContext->device_context.physical_device, pContext->surface, &out_support_info->surface_capabilites);

    vkGetPhysicalDeviceSurfaceFormatsKHR(pContext->device_context.physical_device, pContext->surface, &out_support_info->format_count, 0);
    out_support_info->surface_formats.resize(out_support_info->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pContext->device_context.physical_device, pContext->surface, &out_support_info->format_count, out_support_info->surface_formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(pContext->device_context.physical_device, pContext->surface, &out_support_info->present_mode_count, 0);
    out_support_info->present_modes.resize(out_support_info->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(pContext->device_context.physical_device, pContext->surface, &out_support_info->present_mode_count, out_support_info->present_modes.data());
}

b8 acquire_next_image_index_swapchain(
    VulkanContext* pContext,
    VulkanSwapchain* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* image_index)
{
    VkResult result = vkAcquireNextImageKHR(pContext->device_context.handle, swapchain->handle, timeout_ns, image_available_semaphore, fence, image_index);

    switch (result) {
    case VK_ERROR_OUT_OF_DATE_KHR:
        //TODO
        //vulkan_swapchain_recreate(pContext, pContext->framebuffer_width, pContext->framebuffer_width);
        return false;
    case VK_SUBOPTIMAL_KHR:
        break;
        // if the swapchain no longer matches the surface properties exactly,
        // -but can still be used for presentation
    }

    return true;
}

b8 present_image_swapchain(
    VulkanContext* pContext,
    VulkanSwapchain* swapchain,
    VkQueue mPresentQueue,
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

    VkResult result = vkQueuePresentKHR(mPresentQueue, &present_info);

    switch (result) {
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
        return false;
    }

    return true;
}

void vulkan_framebuffer_create(VulkanContext* pContext, VulkanRenderpass* renderpass, u32 attachment_count, VkImageView* image_view, VkFramebuffer* out_framebuffer)
{
    VkFramebufferCreateInfo create_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    create_info.renderPass = renderpass->handle;
    create_info.attachmentCount = attachment_count;
    create_info.pAttachments = image_view;
    create_info.width = pContext->framebuffer_width;
    create_info.height = pContext->framebuffer_height;
    create_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(
        pContext->device_context.handle,
        &create_info,
        pContext->allocator,
        out_framebuffer
    ));
}

void vulkan_framebuffer_destroy(VulkanContext* pContext, VkFramebuffer* framebuffer) {
    vkDestroyFramebuffer(pContext->device_context.handle, *framebuffer, pContext->allocator);
    framebuffer = 0;
}

bool is_image_format_depth_only(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT: return true;
        default: return false;
    }
}

bool is_image_format_depth_stencil(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT: 
        case VK_FORMAT_D24_UNORM_S8_UINT: return true;
        default: return false;
    }
}

VkSampleCountFlagBits to_vulkan_sample_count(u32 sampleCount)
{
    switch (sampleCount)
    {
        case 1: return   VK_SAMPLE_COUNT_1_BIT;
        case 2: return   VK_SAMPLE_COUNT_2_BIT;
        case 4: return   VK_SAMPLE_COUNT_4_BIT;
        case 8: return   VK_SAMPLE_COUNT_8_BIT;
        case 16: return  VK_SAMPLE_COUNT_16_BIT;
        case 32: return  VK_SAMPLE_COUNT_32_BIT;
        case 64: return  VK_SAMPLE_COUNT_64_BIT;
        default: assert(false); 
    }

    return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
}

VkImageUsageFlags descriptor_type_to_vulkan_image_usage(DescriptorType type)
{
    VkImageUsageFlags flags = 0;

    if (type & DESCRIPTOR_TYPE_TEXTURE)
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if (type & DESCRIPTOR_TYPE_RW_TEXTURE)
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

void vulkan_texture_create(VulkanContext* pContext, TextureDesc* pDesc, Texture** ppTexture)
{
    assert(pContext);
    assert(pDesc);
    assert(ppTexture);

    Texture* pTexture = (Texture*)alloc_aligned_memory(sizeof(Texture) + pDesc->mType & DESCRIPTOR_TYPE_RW_TEXTURE ? sizeof(VkImageView) * pDesc->mMipLevels : 0, 16);
    memset(pTexture, 0, sizeof(sizeof(Texture) + pDesc->mType & DESCRIPTOR_TYPE_RW_TEXTURE ? sizeof(VkImageView) * pDesc->mMipLevels : 0));

    if (pDesc->mType & DESCRIPTOR_TYPE_RW_TEXTURE)
        pTexture->pUAVDescriptors = (VkImageView*)(pTexture + 1);

    if (pDesc->pNativeHandle == NULL)
    {
        pTexture->bOwnsImage = true;
    }
    else
    {
        pTexture->bOwnsImage = false;
        pTexture->pImage = (VkImage)pDesc->pNativeHandle;
    }
    
    pTexture->mWidth = pDesc->mWidth;
    pTexture->mHeight = pDesc->mHeight;
    pTexture->mFormat = pDesc->mFormat;
    pTexture->mSampleCount = pDesc->mSampleCount;
    pTexture->mMipLevels = pDesc->mMipLevels;

    if (pTexture->pImage == VK_NULL_HANDLE)
    {
        VkImageCreateInfo imgCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imgCreateInfo.pNext = NULL;
        imgCreateInfo.flags = 0;
        imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imgCreateInfo.format = pDesc->mFormat;
        imgCreateInfo.extent.width = pDesc->mWidth;
        imgCreateInfo.extent.height = pDesc->mHeight;
        imgCreateInfo.extent.depth = 1;
        imgCreateInfo.mipLevels = pDesc->mMipLevels;
        imgCreateInfo.arrayLayers = 1;
        imgCreateInfo.samples = to_vulkan_sample_count(pDesc->mSampleCount);
        imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgCreateInfo.usage = descriptor_type_to_vulkan_image_usage(pDesc->mType) | resource_state_to_vulkan_image_usage(pDesc->mStartState);
        imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imgCreateInfo.queueFamilyIndexCount = 0;
        imgCreateInfo.pQueueFamilyIndices = nullptr;
        imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VK_CHECK(vmaCreateImage(pContext->vma_allocator, &imgCreateInfo, &allocCreateInfo, &pTexture->pImage, &pTexture->pAlloc, nullptr));
    }
    
    // SRV
    VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = pTexture->pImage;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = pDesc->mFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = pDesc->mMipLevels;
    viewCreateInfo.subresourceRange.aspectMask = format_to_vulkan_image_aspect(pDesc->mFormat);
    pTexture->mAspectMask = format_to_vulkan_image_aspect(pDesc->mFormat);

    if (pDesc->mType & DESCRIPTOR_TYPE_TEXTURE)
    {
        VK_CHECK(vkCreateImageView(pContext->device_context.handle, &viewCreateInfo, pContext->allocator, &pTexture->pSRVDescriptor));
    }

    if (pDesc->mType & DESCRIPTOR_TYPE_RW_TEXTURE)
    {
        viewCreateInfo.subresourceRange.levelCount = 1;
        for (u32 i = 0; i < pDesc->mMipLevels; ++i)
        {
            viewCreateInfo.subresourceRange.baseMipLevel = i;
            VK_CHECK(vkCreateImageView(pContext->device_context.handle, &viewCreateInfo, pContext->allocator, &pTexture->pUAVDescriptors[i]));
        }
    }

    *ppTexture = pTexture;
}

void vulkan_texture_destroy(VulkanContext* pContext, Texture* pTexture)
{
    assert(pContext);
    assert(pTexture);

    if (pTexture->pSRVDescriptor != VK_NULL_HANDLE)
    {
        vkDestroyImageView(pContext->device_context.handle, pTexture->pSRVDescriptor, pContext->allocator);
    }

    if (pTexture->pUAVDescriptors)
    {
        for (u32 i = 0; i < pTexture->mMipLevels; ++i)
        {
            vkDestroyImageView(pContext->device_context.handle, pTexture->pUAVDescriptors[i], pContext->allocator);
        }
    }

    if (pTexture->pImage != VK_NULL_HANDLE)
        vmaDestroyImage(pContext->vma_allocator, pTexture->pImage, pTexture->pAlloc);
}

b8 load_image_from_file(VulkanContext* pContext, const char* file, Texture* pTexture)
{
    i32 tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(file, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "failed to load texture file " << file << std::endl;
        return false;
    }

    void* pixel_ptr = pixels;

    Buffer staging_buffer;
    u32 staging_buffer_size = tex_width * tex_height * 4;

    vulkan_buffer_create(pContext, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &staging_buffer);
    vulkan_buffer_upload(pContext, &staging_buffer, pixel_ptr, staging_buffer_size);
    stbi_image_free(pixels);

    //vulkan_image_create(
    //    pContext, VK_IMAGE_TYPE_2D,
    //    tex_width, tex_height,
    //    VK_FORMAT_R8G8B8A8_SRGB,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    //    VMA_MEMORY_USAGE_GPU_ONLY,
    //    true,
    //    VK_IMAGE_ASPECT_COLOR_BIT,
    //    out_image
    //);

    // create one time submit command buffer for image copy from staging buffer
    Command one_time_submit;

    vulkan_command_pool_create(pContext, &one_time_submit, QUEUE_TYPE_TRANSFER);
    vulkan_command_buffer_allocate(pContext, &one_time_submit, true);

    vulkan_command_buffer_begin(&one_time_submit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //vulkan_image_layout_transition(
    //    out_image,
    //    &one_time_submit,
    //    VK_IMAGE_LAYOUT_UNDEFINED,
    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    0,
    //    VK_ACCESS_TRANSFER_WRITE_BIT,
    //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //    VK_PIPELINE_STAGE_TRANSFER_BIT
    //);

    VkBufferImageCopy copy_region = {};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;

    //TODO: mipmap
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageExtent = { pTexture->mWidth, pTexture->mHeight, 1 };

    //copy the buffer into the image
    vkCmdCopyBufferToImage(one_time_submit.buffer, staging_buffer.handle, pTexture->pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    //vulkan_image_layout_transition(
    //    out_image,
    //    &one_time_submit,
    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //    VK_ACCESS_TRANSFER_WRITE_BIT,
    //    VK_ACCESS_SHADER_READ_BIT,
    //    VK_PIPELINE_STAGE_TRANSFER_BIT,
    //    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    //);

    vulkan_command_buffer_end(&one_time_submit);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &one_time_submit.buffer;

    VK_CHECK(vkQueueSubmit(pContext->device_context.mTransferQueue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(pContext->device_context.mTransferQueue);
    vulkan_command_pool_destroy(pContext, &one_time_submit);

    vulkan_buffer_destroy(pContext, &staging_buffer);

    return true;
}