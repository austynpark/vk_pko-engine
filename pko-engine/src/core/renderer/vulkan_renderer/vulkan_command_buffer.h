#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

struct VulkanContext;
struct Command;

void vulkan_command_pool_create(VulkanContext* pContext, Command* pCmd, QueueType queueType);
void vulkan_command_pool_destroy(VulkanContext* pContext, Command* pCmd);

void vulkan_command_buffer_allocate(VulkanContext* pContext, Command* pCmd, b8 is_primary);
void vulkan_command_buffer_begin(Command* pCmd, VkCommandBufferUsageFlags buffer_usage);
void vulkan_command_buffer_end(Command* pCmd);
void vulkan_command_pool_reset(Command* pCmd);

void vulkan_command_resource_barrier(Command* pCmd,
	BufferBarrier* pBufferBarriers, u32 buffBarrierCount,
	TextureBarrier* pTextureBarriers, u32 textureBarrierCount,
	RenderTargetBarrier* pRenderTargetBarriers, u32 renderTargetBarrierCount
);

#endif // !VULKAN_COMMAND_BUFFER_H
