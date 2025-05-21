#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "vulkan_types.inl"

void vulkan_command_pool_create(RenderContext* context, Command* command,
                                QueueType queueType);
void vulkan_command_pool_destroy(RenderContext* context, Command* command);

void vulkan_command_buffer_allocate(RenderContext* context, Command* command,
                                    b8 is_primary);
void vulkan_command_buffer_begin(Command* command,
                                 VkCommandBufferUsageFlags buffer_usage);
void vulkan_command_buffer_end(Command* command);
void vulkan_command_pool_reset(Command* command);

void vulkan_command_resource_barrier(
    Command* command, BufferBarrier* bufferBarriers, u32 buffBarrierCount,
    TextureBarrier* textureBarriers, u32 textureBarrierCount,
    RenderTargetBarrier* pRenderTargetBarriers, u32 renderTargetBarrierCount);

#endif  // !VULKAN_COMMAND_BUFFER_H
