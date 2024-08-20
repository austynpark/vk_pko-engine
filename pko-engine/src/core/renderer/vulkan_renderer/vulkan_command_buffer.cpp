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
		if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

		if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
		{
			flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		break;
	}
	case QUEUE_TYPE_COMPUTE:
	{
		if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 ||
			(accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
			(accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
			(accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
			return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		break;
	}
	case QUEUE_TYPE_TRANSFER: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	default: break;
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

void vulkan_command_pool_create(VulkanContext* pContext, Command* pCmd, QueueType queueType)
{
	assert(pContext);
	assert(&pContext->device_context);

	VkCommandPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = get_queue_family_index(&pContext->device_context, queueType);
	pCmd->type = queueType;

	VK_CHECK(vkCreateCommandPool(pContext->device_context.handle, &create_info, pContext->allocator, &pCmd->pool));
}

void vulkan_command_pool_destroy(VulkanContext* pContext, Command* pCmd)
{
	assert(pContext);
	assert(pCmd);

	vkDestroyCommandPool(pContext->device_context.handle, pCmd->pool, pContext->allocator);
}

void vulkan_command_buffer_allocate(VulkanContext* pContext, Command* pCmd, b8 is_primary)
{
	assert(pContext);
	assert(pCmd);

	VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool = pCmd->pool;
	alloc_info.commandBufferCount = 1;
	alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VK_CHECK(vkAllocateCommandBuffers(pContext->device_context.handle, &alloc_info, &pCmd->buffer));
}

void vulkan_command_buffer_begin(Command* pCmd, VkCommandBufferUsageFlags buffer_usage)
{
	if (pCmd == nullptr)
		return;

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags = buffer_usage;
	//TODO: for secondary command buffer
	begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(pCmd->buffer, &begin_info));
}

void vulkan_command_buffer_end(Command* pCmd)
{
	if (pCmd == nullptr)
		return;

	VK_CHECK(vkEndCommandBuffer(pCmd->buffer));
}

void vulkan_command_resource_barrier(Command* pCmd, BufferBarrier* pBufferBarriers, u32 buffBarrierCount, TextureBarrier* pTextureBarriers, u32 textureBarrierCount, RenderTargetBarrier* pRenderTargetBarriers, u32 renderTargetBarrierCount)
{
	assert(pCmd);

	VkAccessFlags srcAccessMask = 0;
	VkAccessFlags dstAccessMask = 0;

	u32 imageMemoryBarrierCount = 0;
	VkImageMemoryBarrier* pImageMemoryBarriers = (VkImageMemoryBarrier*)malloc(sizeof(VkImageMemoryBarrier) * imageMemoryBarrierCount);

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

	for (uint32_t i = 0; i < buffBarrierCount; ++i)
	{
		BufferBarrier* buffBarrier = &pBufferBarriers[i];

		if (buffBarrier->currentState == RESOURCE_STATE_UNORDERED_ACCESS && buffBarrier->newState == RESOURCE_STATE_UNORDERED_ACCESS)
		{
			memoryBarrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
			memoryBarrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		}
		else
		{
			memoryBarrier.srcAccessMask |= resource_state_to_access_flags(buffBarrier->currentState);
			memoryBarrier.dstAccessMask |= resource_state_to_access_flags(buffBarrier->newState);
		}

		srcAccessMask |= memoryBarrier.srcAccessMask;
		dstAccessMask |= memoryBarrier.dstAccessMask;
	}

	for (u32 i = 0; i < textureBarrierCount; ++i, ++imageMemoryBarrierCount)
	{
		TextureBarrier* pTextureBarrier = &pTextureBarriers[i];
		Texture* pTexture = pTextureBarrier->pTexture;

		VkImageMemoryBarrier* pImageMemoryBarrier = &pImageMemoryBarriers[imageMemoryBarrierCount];

		if (pImageMemoryBarrier)
		{
			pImageMemoryBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			if (pTextureBarrier->currentState == RESOURCE_STATE_UNORDERED_ACCESS && pTextureBarrier->newState == RESOURCE_STATE_UNORDERED_ACCESS)
			{
				pImageMemoryBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				pImageMemoryBarrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			}
			else
			{
				pImageMemoryBarrier->srcAccessMask = resource_state_to_access_flags(pTextureBarrier->currentState);
				pImageMemoryBarrier->dstAccessMask = resource_state_to_access_flags(pTextureBarrier->newState);
			}

			pImageMemoryBarrier->oldLayout = resource_state_to_vulkan_image_layout(pTextureBarrier->currentState);
			pImageMemoryBarrier->newLayout = resource_state_to_vulkan_image_layout(pTextureBarrier->newState);
			pImageMemoryBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pImageMemoryBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pImageMemoryBarrier->image = pTexture->pImage;

			pImageMemoryBarrier->subresourceRange.aspectMask = pTexture->aspectMask;;
			pImageMemoryBarrier->subresourceRange.baseMipLevel = 0;
			pImageMemoryBarrier->subresourceRange.levelCount = 1;
			pImageMemoryBarrier->subresourceRange.baseArrayLayer = 0;
			pImageMemoryBarrier->subresourceRange.layerCount = 1;

			srcAccessMask |= pImageMemoryBarrier->srcAccessMask;
			dstAccessMask |= pImageMemoryBarrier->dstAccessMask;
		}
	}

	for (u32 i = 0; i < renderTargetBarrierCount; ++i, ++imageMemoryBarrierCount)
	{
		RenderTargetBarrier* pRenderTargetBarrier = &pRenderTargetBarriers[i];
		Texture* pTexture = pRenderTargetBarrier->pRenderTarget->pTexture;

		VkImageMemoryBarrier* pImageMemoryBarrier = &pImageMemoryBarriers[imageMemoryBarrierCount];

		if (pImageMemoryBarrier)
		{
			pImageMemoryBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			if (pRenderTargetBarrier->currentState == RESOURCE_STATE_UNORDERED_ACCESS && pRenderTargetBarrier->newState == RESOURCE_STATE_UNORDERED_ACCESS)
			{
				pImageMemoryBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				pImageMemoryBarrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			}
			else
			{
				pImageMemoryBarrier->srcAccessMask = resource_state_to_access_flags(pRenderTargetBarrier->currentState);
				pImageMemoryBarrier->dstAccessMask = resource_state_to_access_flags(pRenderTargetBarrier->newState);
			}

			pImageMemoryBarrier->oldLayout = resource_state_to_vulkan_image_layout(pRenderTargetBarrier->currentState);
			pImageMemoryBarrier->newLayout = resource_state_to_vulkan_image_layout(pRenderTargetBarrier->newState);
			pImageMemoryBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pImageMemoryBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pImageMemoryBarrier->image = pTexture->pImage;

			pImageMemoryBarrier->subresourceRange.aspectMask = pTexture->aspectMask;
			pImageMemoryBarrier->subresourceRange.baseMipLevel = 0;
			pImageMemoryBarrier->subresourceRange.levelCount = 1;
			pImageMemoryBarrier->subresourceRange.baseArrayLayer = 0;
			pImageMemoryBarrier->subresourceRange.layerCount = 1;

			srcAccessMask |= pImageMemoryBarrier->srcAccessMask;
			dstAccessMask |= pImageMemoryBarrier->dstAccessMask;
		}
	}

	VkPipelineStageFlags srcStageMask = get_pipeline_stage_flags(srcAccessMask, pCmd->type);
	VkPipelineStageFlags dstStageMask = get_pipeline_stage_flags(dstAccessMask, pCmd->type);

	if (srcAccessMask != 0 || dstAccessMask != 0)
	{
		vkCmdPipelineBarrier(pCmd->buffer, srcStageMask, dstStageMask, 0, 1, &memoryBarrier, 0, nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
	}
}


