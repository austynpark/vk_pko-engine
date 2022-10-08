#include "vulkan_scene.h"

#include "../vulkan_device.h"
#include "../vulkan_memory_allocate.h"
#include "../vulkan_swapchain.h"
#include "../vulkan_command_buffer.h"
#include "../vulkan_renderpass.h"
#include "../vulkan_framebuffer.h"
#include "../vulkan_pipeline.h"
#include "../vulkan_mesh.h"
#include "../vulkan_skinned_mesh.h"
#include "../vulkan_buffer.h"
#include "../vulkan_shader.h"
#include "../vulkan_scene/vulkan_scene.h"

#include <iostream>

vulkan_scene::vulkan_scene()
{
}

vulkan_scene::~vulkan_scene()
{
}

b8 vulkan_scene::init(vulkan_context* api_context)
{
    if (api_context == nullptr) {
        return false;
    }

    context = api_context;

    // create command pool per frame
    graphics_commands.resize(context->swapchain.max_frames_in_flight);
	for (u32 i = 0; i < context->swapchain.max_frames_in_flight; ++i) {
		vulkan_command_pool_create(context, &graphics_commands.at(i), context->device_context.graphics_family.index);
		vulkan_command_buffer_allocate(context, &graphics_commands.at(i), true);
    }

	main_shader = std::make_unique<vulkan_shader>(api_context, &context->main_renderpass);

    framebuffers.resize(context->swapchain.image_count);

    regenerate_framebuffer();

    std::cout << "framebuffers created" << std::endl;

    object_manager["boblampclean"] = std::make_unique<skinned_mesh>(context, "model/boblampclean.md5mesh");
    object_manager["boblampclean"]->position = glm::vec3(0.0f, -1.5f, 0.0f);
    object_manager["boblampclean"]->scale = glm::vec3(0.1f);
    object_manager["boblampclean"]->rotation = glm::vec3(-90.0f, 0.0f, 0.0f);

    object_manager["boxguy"] = std::make_unique<skinned_mesh>(context, "model/boxguy.fbx");
    object_manager["boxguy"]->position = glm::vec3(0, -5.0f, -10.0f);
    object_manager["boxguy"]->scale = glm::vec3(0.1f);
    object_manager["boxguy"]->rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    object_manager["tad"] = std::make_unique<skinned_mesh>(context, "model/FBX 2013/Tad.fbx");

    //object_manager["giant"] = std::make_unique<skinned_mesh>(context, "model/giant.fbx");
    //object_manager["sponza"] = std::make_unique<vulkan_render_object>(context, "model/sponza.obj");

    return true;
}

b8 vulkan_scene::begin_frame(f32 dt)
{
    //vulkan_context* context = reinterpret_cast<vulkan_context*>(api_context);

    return true;
}

b8 vulkan_scene::end_frame()
{
    //vulkan_context* context = reinterpret_cast<vulkan_context*>(api_context);
    return true;
}

b8 vulkan_scene::begin_renderpass(u32 current_frame)
{
    //TODO: multiple command buffers
    VkCommandBuffer command_buffer = graphics_commands.at(current_frame).buffer;

    VkClearValue clear_values[] = {
        // color
        { {1.0f, 1.0f, 1.0f, 1.0f} },
        // depth, stencil
        {{1.0f, 0.0f}}
    };

    VkRenderPassBeginInfo renderpass_begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderpass_begin_info.renderPass = context->main_renderpass.handle;
    renderpass_begin_info.framebuffer = framebuffers.at(image_index);
    renderpass_begin_info.renderArea.extent = { context->main_renderpass.width, context->main_renderpass.height };
    renderpass_begin_info.renderArea.offset.x = context->main_renderpass.x;
    renderpass_begin_info.renderArea.offset.y = context->main_renderpass.y;
    renderpass_begin_info.clearValueCount = 2;
    renderpass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    return true;
}

b8 vulkan_scene::end_renderpass(u32 current_frame)
{
    //TODO: multiple command buffers
    VkCommandBuffer command_buffer = graphics_commands.at(current_frame).buffer;

    vkCmdEndRenderPass(command_buffer);

    return true;
}

b8 vulkan_scene::draw()
{
    u32 current_frame = context->current_frame;
    float framebuffer_width = context->framebuffer_width;
    float framebuffer_height = context->framebuffer_height;

    VkCommandBuffer command_buffer = graphics_commands[current_frame].buffer;
    float    x;
    float    y;
    float    width;
    float    height;
    float    minDepth;
    float    maxDepth;

    VkViewport viewport = {
        0, 0,
        framebuffer_width, framebuffer_height,
        0.0f, 1.0f
    };

    VkRect2D scissor = {
        0, 0,
        (u32)framebuffer_width, (u32)framebuffer_height
    };

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    return true;
}

b8 vulkan_scene::draw_imgui()
{
    return true;
}

void vulkan_scene::shutdown()
{
    main_shader->shutdown();
    

	for (auto& obj : object_manager) {
        obj.second->destroy();
    }

    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(context, &framebuffers.at(i));
    }

    for (u32 i = 0; i < context->swapchain.max_frames_in_flight; ++i)
        vulkan_command_pool_destroy(context, &graphics_commands.at(i));
}

b8 vulkan_scene::on_resize(u32 w, u32 h)
{
    context->main_renderpass.width = w;
    context->main_renderpass.height = h;

    regenerate_framebuffer();

    return true;
}

void vulkan_scene::regenerate_framebuffer()
{
    for (u32 i = 0; i < context->swapchain.image_count; ++i) {

        VkImageView image_views[] = {
            context->swapchain.image_views.at(i),
            context->swapchain.depth_attachment.view
        };

        u32 attachment_count = 2;

        vulkan_framebuffer_create(context, &context->main_renderpass, attachment_count, image_views, &framebuffers.at(i));
    }
}

