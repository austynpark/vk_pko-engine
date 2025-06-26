#include "vulkan_command_buffer.h"

#include "vulkan_types.inl"
#include "vulkan_image.h"

u32 get_queue_family_index(DeviceContext* pDevice, QueueType queueType)
{
    assert(pDevice);

    u32 result = -1;
    switch (queueType)
    {
        case QUEUE_TYPE_GRAPHICS:
            result = pDevice->graphics_family.index;
    }

    return result;
}

VkPipelineStageFlags get_pipeline_stage_flags(VkAccessFlags accessFlags, QueueType queueType)
{
    VkPipelineStageFlags flags = 0;

    switch (queueType)
    {
        case QUEUE_TYPE_GRAPHICS:
        {
            if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) !=
                0)
                flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

            if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                                VK_ACCESS_SHADER_WRITE_BIT)) != 0)
            {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            if ((accessFlags &
                 (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
                flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            break;
        }
        case QUEUE_TYPE_COMPUTE:
        {
            if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) !=
                    0 ||
                (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
                (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
                (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                                VK_ACCESS_SHADER_WRITE_BIT)) != 0)
                flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            break;
        }
        case QUEUE_TYPE_TRANSFER:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        default:
            break;
    }

    if ((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
}

VkAccessFlags resource_state_to_access_flags(ResourceState state)
{
    VkAccessFlags flags = 0;
    if (state & RESOURCE_STATE_COPY_SOURCE)
    {
        flags |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (state & RESOURCE_STATE_COPY_DEST)
    {
        flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
    {
        flags |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (state & RESOURCE_STATE_INDEX_BUFFER)
    {
        flags |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (state & RESOURCE_STATE_UNORDERED_ACCESS)
    {
        flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_RENDER_TARGET)
    {
        flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_DEPTH_WRITE)
    {
        flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_DEPTH_READ)
    {
        flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (state & RESOURCE_STATE_SHADER_RESOURCE)
    {
        flags |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (state & RESOURCE_STATE_PRESENT)
    {
        flags |= VK_ACCESS_MEMORY_READ_BIT;
    }

    return flags;
}

void vulkan_command_pool_create(RenderContext* context, Command* command, QueueType queueType)
{
    assert(context);
    assert(&context->device_context);

    VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = get_queue_family_index(&context->device_context, queueType);
    command->type = queueType;
    command->is_rendering = false;

    VK_CHECK(vkCreateCommandPool(context->device_context.handle, &create_info, context->allocator,
                                 &command->pool));
}

void vulkan_command_pool_destroy(RenderContext* context, Command* command)
{
    assert(context);
    assert(command);

    vkDestroyCommandPool(context->device_context.handle, command->pool, context->allocator);
}

void vulkan_command_buffer_allocate(RenderContext* context, Command* command, b8 is_primary)
{
    assert(context);
    assert(command);

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = command->pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level =
        is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

    VK_CHECK(
        vkAllocateCommandBuffers(context->device_context.handle, &alloc_info, &command->buffer));
}

void vulkan_command_buffer_begin(Command* command, VkCommandBufferUsageFlags buffer_usage)
{
    if (command == NULL)
        return;

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = buffer_usage;
    // TODO: for secondary command buffer
    begin_info.pInheritanceInfo = NULL;

    VK_CHECK(vkBeginCommandBuffer(command->buffer, &begin_info));
}

void vulkan_command_buffer_end(Command* command)
{
    if (command == NULL)
        return;

    if (command->is_rendering)
    {
        vkCmdEndRenderingKHR(command->buffer);
        command->is_rendering = false;
    }

    VK_CHECK(vkEndCommandBuffer(command->buffer));
}

void vulkan_command_pool_reset(Command* command)
{
    if (command == NULL)
        return;

    VK_CHECK(vkResetCommandBuffer(command->buffer, 0));
}

void vulkan_command_resource_barrier(Command* command, BufferBarrier* bufferBarriers,
                                     u32 buffBarrierCount, TextureBarrier* textureBarriers,
                                     u32 textureBarrierCount,
                                     RenderTargetBarrier* pRenderTargetBarriers,
                                     u32 renderTargetBarrierCount)
{
    assert(command);

    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;

    u32 imageMemoryBarrierCount = 0;
    VkImageMemoryBarrier* imageMemoryBarriers = (VkImageMemoryBarrier*)calloc(
        textureBarrierCount + renderTargetBarrierCount, sizeof(VkImageMemoryBarrier));

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

    for (uint32_t i = 0; i < buffBarrierCount; ++i)
    {
        BufferBarrier* buffBarrier = &bufferBarriers[i];

        if (buffBarrier->current_state == RESOURCE_STATE_UNORDERED_ACCESS &&
            buffBarrier->new_state == RESOURCE_STATE_UNORDERED_ACCESS)
        {
            memoryBarrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
            memoryBarrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        }
        else
        {
            memoryBarrier.srcAccessMask |=
                resource_state_to_access_flags(buffBarrier->current_state);
            memoryBarrier.dstAccessMask |= resource_state_to_access_flags(buffBarrier->new_state);
        }

        srcAccessMask |= memoryBarrier.srcAccessMask;
        dstAccessMask |= memoryBarrier.dstAccessMask;
    }

    for (u32 i = 0; i < textureBarrierCount; ++i, ++imageMemoryBarrierCount)
    {
        TextureBarrier* textureBarrier = &textureBarriers[i];
        Texture* texture = textureBarrier->texture;

        VkImageMemoryBarrier* imageMemoryBarrier = &imageMemoryBarriers[imageMemoryBarrierCount];
        if (imageMemoryBarrier)
        {
            imageMemoryBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            if (textureBarrier->current_state == RESOURCE_STATE_UNORDERED_ACCESS &&
                textureBarrier->new_state == RESOURCE_STATE_UNORDERED_ACCESS)
            {
                imageMemoryBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                imageMemoryBarrier->dstAccessMask =
                    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }
            else
            {
                imageMemoryBarrier->srcAccessMask =
                    resource_state_to_access_flags(textureBarrier->current_state);
                imageMemoryBarrier->dstAccessMask =
                    resource_state_to_access_flags(textureBarrier->new_state);
            }

            imageMemoryBarrier->oldLayout =
                resource_state_to_vulkan_image_layout(textureBarrier->current_state);
            imageMemoryBarrier->newLayout =
                resource_state_to_vulkan_image_layout(textureBarrier->new_state);
            imageMemoryBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier->image = texture->image;

            imageMemoryBarrier->subresourceRange.aspectMask = texture->aspect_mask;

            imageMemoryBarrier->subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier->subresourceRange.levelCount = 1;
            imageMemoryBarrier->subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier->subresourceRange.layerCount = 1;

            srcAccessMask |= imageMemoryBarrier->srcAccessMask;
            dstAccessMask |= imageMemoryBarrier->dstAccessMask;
        }
    }

    for (u32 i = 0; i < renderTargetBarrierCount; ++i, ++imageMemoryBarrierCount)
    {
        RenderTargetBarrier* pRenderTargetBarrier = &pRenderTargetBarriers[i];
        Texture* texture = pRenderTargetBarrier->render_target->texture;

        VkImageMemoryBarrier* imageMemoryBarrier = &imageMemoryBarriers[imageMemoryBarrierCount];

        if (imageMemoryBarrier)
        {
            imageMemoryBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            if (pRenderTargetBarrier->current_state == RESOURCE_STATE_UNORDERED_ACCESS &&
                pRenderTargetBarrier->new_state == RESOURCE_STATE_UNORDERED_ACCESS)
            {
                imageMemoryBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                imageMemoryBarrier->dstAccessMask =
                    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }
            else
            {
                imageMemoryBarrier->srcAccessMask =
                    resource_state_to_access_flags(pRenderTargetBarrier->current_state);
                imageMemoryBarrier->dstAccessMask =
                    resource_state_to_access_flags(pRenderTargetBarrier->new_state);
            }

            imageMemoryBarrier->oldLayout =
                resource_state_to_vulkan_image_layout(pRenderTargetBarrier->current_state);
            imageMemoryBarrier->newLayout =
                resource_state_to_vulkan_image_layout(pRenderTargetBarrier->new_state);
            imageMemoryBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier->image = texture->image;

            imageMemoryBarrier->subresourceRange.aspectMask = texture->aspect_mask;
            imageMemoryBarrier->subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier->subresourceRange.levelCount = 1;
            imageMemoryBarrier->subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier->subresourceRange.layerCount = 1;

            srcAccessMask |= imageMemoryBarrier->srcAccessMask;
            dstAccessMask |= imageMemoryBarrier->dstAccessMask;
        }
    }

    assert(imageMemoryBarrierCount == textureBarrierCount + renderTargetBarrierCount);

    VkPipelineStageFlags srcStageMask = get_pipeline_stage_flags(srcAccessMask, command->type);
    VkPipelineStageFlags dstStageMask = get_pipeline_stage_flags(dstAccessMask, command->type);

    if (srcAccessMask != 0 || dstAccessMask != 0)
    {
        vkCmdPipelineBarrier(command->buffer, srcStageMask, dstStageMask, 0, 1, &memoryBarrier, 0,
                             nullptr, imageMemoryBarrierCount, imageMemoryBarriers);
    }

    if (imageMemoryBarriers != NULL)
    {
        free(imageMemoryBarriers);
        imageMemoryBarriers = NULL;
    }
}

void vulkan_command_buffer_rendering(Command* command, RenderDesc* desc)
{
    assert(command);

    if (command->is_rendering)
    {
        vkCmdEndRenderingKHR(command->buffer);
        command->is_rendering = false;
    }

    if (desc == NULL)
    {
        return;
    }

    assert(desc);
    assert(desc->render_target_count <= MAX_COLOR_ATTACHMENT);

    u32 render_target_idx = 0;
    VkRenderingAttachmentInfoKHR color_attachment_info[MAX_COLOR_ATTACHMENT] = {};
    VkRenderingAttachmentInfoKHR depth_attachment = {};
    VkRenderingAttachmentInfoKHR stencil_attachment = {};

    for (render_target_idx = 0; render_target_idx < desc->render_target_count; ++render_target_idx)
    {
        VkRenderingAttachmentInfoKHR& color_attachment = color_attachment_info[render_target_idx];
        const RenderTarget* render_target = desc->render_targets[render_target_idx];
        const RenderTargetOperator& render_target_op =
            desc->render_target_operators[render_target_idx];
        color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        color_attachment.imageView = render_target->descriptor;
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = render_target_op.load_op;
        color_attachment.storeOp = render_target_op.store_op;
        color_attachment.clearValue = desc->clear_color;
    }

    const RenderTarget* depth_rt = desc->depth_target;
    const RenderTarget* stencil_rt = desc->stencil_target;

    if (depth_rt != NULL)
    {
        const RenderTargetOperator& render_target_op =
            desc->render_target_operators[render_target_idx++];

        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depth_attachment.imageView = depth_rt->descriptor;
        depth_attachment.imageLayout = desc->is_depth_stencil
                                           ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                           : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = render_target_op.load_op;
        depth_attachment.storeOp = render_target_op.store_op;
        depth_attachment.clearValue = desc->clear_depth;
    }

    if (stencil_rt != NULL)
    {
        const RenderTargetOperator& render_target_op =
            desc->render_target_operators[render_target_idx];

        stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        stencil_attachment.imageView = stencil_rt->descriptor;
        stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        stencil_attachment.loadOp = render_target_op.load_op;
        stencil_attachment.storeOp = render_target_op.store_op;
        stencil_attachment.clearValue = desc->clear_depth;
    }

    VkRenderingInfoKHR rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    rendering_info.renderArea = desc->render_area;
    rendering_info.layerCount = 1;  // TODO
    rendering_info.colorAttachmentCount = desc->render_target_count;
    rendering_info.pColorAttachments = color_attachment_info;
    rendering_info.pDepthAttachment = depth_rt ? &depth_attachment : VK_NULL_HANDLE;
    rendering_info.pStencilAttachment = stencil_rt ? &stencil_attachment : VK_NULL_HANDLE;

    if (!command->is_rendering)
    {
        vkCmdBeginRenderingKHR(command->buffer, &rendering_info);
        command->is_rendering = true;
    }
}