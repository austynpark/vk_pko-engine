#include "vulkan_renderer.h"

#define VMA_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "vulkan_types.inl"

#include "core/application.h"
#include "core/event.h"
#include "core/input.h"
#include "core/renderer/camera.h"
#include "platform/platform.h"
#include "vendor/mmgr/mmgr.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_memory_allocate.h"
#include "vulkan_pipeline.h"
#include "vulkan_shader.h"
#include "vulkan_mesh.h"

#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"

#include "stb_ds.h"

static vulkan_library vulkan_library_loader{};
static RenderContext context{};

Command cmds[MAX_FRAME];

Mesh* mesh = NULL;
Texture* textures = NULL;

VulkanSwapchain* swapchain = NULL;
VkSemaphore ready_to_render_semaphores[MAX_FRAME];
VkSemaphore image_available_semaphores[MAX_FRAME];
VkFence render_fences[MAX_FRAME];

RenderTarget* depth_render_target = NULL;

enum GBufferTarget
{
    GBUFFER_ALBEDO = 0,
    GBUFFER_NORMAL,
    GBUFFER_MR,
    GBUFFER_COUNT
};

static RenderTarget* gbuffer_render_targets[GBUFFER_COUNT] = {};
static b8 use_gbuffer = true;

PipelineLayout* main_pipeline_layout = NULL;
Pipeline* main_pipeline = NULL;
Pipeline* gbuffer_pipeline = NULL;
Shader* main_shader = NULL;
static Camera main_camera{};

// De-interleaved mesh GPU buffers and counts (globals for this sample)
Buffer position_buffer{};
Buffer normal_buffer{};
Buffer uv_buffer{};
Buffer index_buffer{};
u32 vertex_count = 0;
u32 index_count = 0;

// Scene UBOs and descriptor
struct CameraUBO
{
    glm::mat4 view_proj;
};

struct ModelUBO
{
    glm::mat4 model;
    glm::mat3 normal_matrix;
};

struct DrawData
{
    glm::mat4 model;
    u32 material_index;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};

struct DrawPushConstants
{
    u32 draw_id;
    u32 material_index;
    u32 packed_material;
};

Buffer camera_ubo{};
Buffer draw_data_buffer{};
u32 draw_count = 0;
VkSampler texture_sampler = VK_NULL_HANDLE;
VkDescriptorPool scene_desc_pool = VK_NULL_HANDLE;
VkDescriptorSet scene_desc_set = VK_NULL_HANDLE;
VkDescriptorPool gbuffer_desc_pool = VK_NULL_HANDLE;
VkDescriptorSet gbuffer_desc_set = VK_NULL_HANDLE;

void drawImgui();

static VKAPI_ATTR VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                         void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            std::cout << ("error: %s", pCallbackData->pMessage) << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            std::cout << ("warning: %s", pCallbackData->pMessage) << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cout << ("info: %s", pCallbackData->pMessage) << std::endl;
            break;
        default:
            std::cout << ("info: %s", pCallbackData->pMessage) << std::endl;
    }

    return VK_FALSE;
}

b8 load_exported_entry_points();
b8 load_global_level_function();
b8 load_instance_level_function();
b8 load_device_level_function();

VulkanRenderer::VulkanRenderer(AppState* state) : Renderer(state) {}

VulkanRenderer::~VulkanRenderer()
{
#if defined _WIN32
    FreeLibrary(vulkan_library_loader);
#elif defined _linux
#endif

    vulkan_library_loader = 0;
}

b8 VulkanRenderer::Init()
{
    assert(app_state_);

    context = {};

#if defined(_WIN32)
    vulkan_library_loader = LoadLibrary("vulkan-1.dll");
#endif

    if (vulkan_library_loader == 0)
    {
        std::cout << "vulkan library failed to load" << std::endl;
        return false;
    }

    if (!load_exported_entry_points())
        return false;

    if (!load_global_level_function())
        return false;

    if (!createInstance())
    {
        std::cout << "create instance failed" << std::endl;
        return false;
    }

    if (!load_instance_level_function())
        return false;

    if (!createSurface())
    {
        std::cout << "create surface failed" << std::endl;
        return false;
    }

    if (!vulkan_device_create(&context, &context.device_context))
    {
        std::cout << "create device failed" << std::endl;
        return false;
    }

    if (!load_device_level_function())
        return false;

    vulkan_memory_allocator_create(&context);

    vulkan_get_device_queue(&context.device_context);

    // validation debug logger create
#if defined(_DEBUG)
    createDebugUtilMessage();
#endif

    SwapchainDesc swapchainDesc = {};
    swapchainDesc.width = app_state_->width;
    swapchainDesc.height = app_state_->height;
    swapchainDesc.phWnd = app_state_->platform_state->GetInternalState().handle;
    swapchainDesc.image_count = MAX_FRAME;

    if (!vulkan_swapchain_create(&context, &swapchainDesc, &swapchain))
    {
        std::cout << "create swapchain failed" << std::endl;
        return false;
    }

    initImgui();

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info,
                                   context.allocator, &image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info,
                                   context.allocator, &ready_to_render_semaphores[i]));
        VkFenceCreateInfo fence_create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(context.device_context.handle, &fence_create_info, context.allocator,
                      &render_fences[i]);
    }
    std::cout << "sync objects created" << std::endl;

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        vulkan_command_pool_create(&context, &cmds[i], QUEUE_TYPE_GRAPHICS);
        vulkan_command_buffer_allocate(&context, &cmds[i], true);
    }

    // Load resources
    load_gltf_from_file(&context, "model/main_sponza/NewSponza_Main_glTF_003.gltf", &mesh,
                        &textures);

    // Create de-interleaved GPU buffers from loaded mesh
    if (mesh && arrlen(mesh) > 0)
    {
        create_deinterleaved_mesh_buffers(&context, &mesh[0], &position_buffer, &normal_buffer,
                                          &uv_buffer, &index_buffer, &vertex_count, &index_count);
    }

    return true;
}

void VulkanRenderer::Load(ReloadDesc* desc)
{
    ReloadType reload_type = desc->type;

    const bool reload_all = (reload_type == RELOAD_TYPE_ALL);
    const bool reload_resize = (reload_type & RELOAD_TYPE_RESIZE) == RELOAD_TYPE_RESIZE;
    const bool reload_shader = (reload_type & RELOAD_TYPE_SHADER) == RELOAD_TYPE_SHADER;

    if (reload_all || reload_shader)
    {
        destroyShader();
        createShader();
    }

    if (reload_all || reload_resize || reload_shader)
    {
        destroyGBufferDescriptors();
        destroySceneDescriptors();
        destroyPipeline();
    }

    if (reload_all || reload_resize)
    {
        destroyRenderTarget();
        createRenderTarget();
    }

    if (reload_all || reload_resize || reload_shader)
    {
        createPipeline();
    }

    if (reload_all)
    {
        destroyBuffer();
        createBuffer();
    }

    if (reload_all || reload_resize || reload_shader)
    {
        createSceneDescriptors();
        createGBufferDescriptors();
    }
}

void VulkanRenderer::UnLoad(ReloadDesc* desc)
{
    ReloadType reload_type = desc->type;

    const bool reload_all = (reload_type == RELOAD_TYPE_ALL);
    const bool reload_resize = (reload_type & RELOAD_TYPE_RESIZE) == RELOAD_TYPE_RESIZE;
    const bool reload_shader = (reload_type & RELOAD_TYPE_SHADER) == RELOAD_TYPE_SHADER;

    if (reload_all || reload_resize || reload_shader)
    {
        destroyGBufferDescriptors();
        destroySceneDescriptors();
        destroyPipeline();
    }

    if (reload_all || reload_resize)
    {
        destroyRenderTarget();
    }

    if (reload_all || reload_shader)
    {
        destroyShader();
    }

    if (reload_all)
    {
        destroyBuffer();
    }
}

void VulkanRenderer::Update(const RenderFrameData& frame_data)
{
    CameraUBO camera_data{};
    camera_data.view_proj = frame_data.camera.view_proj;
    vulkan_buffer_upload(&context, &camera_ubo, &camera_data, sizeof(CameraUBO));
}

b8 VulkanRenderer::beginFrame()
{
    context.current_frame = frame_number_++ % MAX_FRAME;
    VK_CHECK(vkWaitForFences(context.device_context.handle, 1,
                             &render_fences[context.current_frame], true, UINT64_MAX));
    VK_CHECK(
        vkResetFences(context.device_context.handle, 1, &render_fences[context.current_frame]));

    if (!acquire_next_image_index_swapchain(&context, swapchain, UINT64_MAX,
                                            image_available_semaphores[context.current_frame], 0,
                                            &context.image_index))
    {
        std::cout << "image acquire failed" << std::endl;
        return false;
    }

    Command* command = &cmds[context.current_frame];
    vulkan_command_pool_reset(command);
    vulkan_command_buffer_begin(command, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return true;
}

void VulkanRenderer::basePass()
{
    Command* command = &cmds[context.current_frame];
    RenderTarget* rendertarget = swapchain->render_targets[context.current_frame];

    // Present to RenderTarget (swapchain only). For GBuffer, no swapchain barrier is needed here.
    TextureBarrier textureBarrier{};
    u32 texture_barrier_count = 0;
    if (!use_gbuffer)
    {
        textureBarrier.current_state = RESOURCE_STATE_PRESENT;
        textureBarrier.new_state = RESOURCE_STATE_RENDER_TARGET;
        textureBarrier.texture = rendertarget->texture;
        texture_barrier_count = 1;
    }

    RenderTargetBarrier render_target_barriers[1]{};
    u32 render_target_barrier_count = 0;
    if (depth_render_target)
    {
        render_target_barriers[0].render_target = depth_render_target;
        render_target_barriers[0].current_state = RESOURCE_STATE_DEPTH_WRITE;
        render_target_barriers[0].new_state = RESOURCE_STATE_DEPTH_WRITE;
        render_target_barrier_count = 1;
    }

    vulkan_command_resource_barrier(command, NULL, 0,
                                    texture_barrier_count ? &textureBarrier : NULL,
                                    texture_barrier_count,
                                    render_target_barriers, render_target_barrier_count);

    RenderTarget* color_targets[GBUFFER_COUNT];
    u32 color_target_count = 0;
    if (use_gbuffer && gbuffer_render_targets[0] != nullptr)
    {
        for (u32 i = 0; i < GBUFFER_COUNT; ++i)
        {
            color_targets[i] = gbuffer_render_targets[i];
        }
        color_target_count = GBUFFER_COUNT;
    }
    else
    {
        color_targets[0] = rendertarget;
        color_target_count = 1;
    }

    RenderTargetOperator rendertarget_ops[MAX_COLOR_ATTACHMENT + 1];
    for (u32 i = 0; i < color_target_count; ++i)
    {
        rendertarget_ops[i].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        rendertarget_ops[i].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    }
    // Depth operator is placed after color ops.
    rendertarget_ops[color_target_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rendertarget_ops[color_target_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    RenderDesc render_desc{};
    render_desc.render_targets = color_targets;
    render_desc.render_target_count = color_target_count;
    render_desc.clear_color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    render_desc.render_area = {{0, 0}, {app_state_->width, app_state_->height}};
    render_desc.render_target_operators = rendertarget_ops;
    render_desc.depth_target = depth_render_target;
    render_desc.clear_depth = {{1.0f, 0U}};
    render_desc.is_depth_stencil = false;

    vulkan_command_buffer_rendering(command, &render_desc);
    VkViewport viewport = {0, 0, app_state_->width, app_state_->height, 0.f, 1.f};
    vkCmdSetViewport(command->buffer, 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, {app_state_->width, app_state_->height}};
    vkCmdSetScissor(command->buffer, 0, 1, &scissor);
    Pipeline* active_pipeline = use_gbuffer ? gbuffer_pipeline : main_pipeline;
    if (active_pipeline)
    {
        vkCmdBindPipeline(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          active_pipeline->handle);
    }

    if (scene_desc_set != VK_NULL_HANDLE && main_pipeline_layout)
    {
        vkCmdBindDescriptorSets(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                main_pipeline_layout->handle, 0, 1, &scene_desc_set, 0, NULL);
    }

    if (position_buffer.handle != VK_NULL_HANDLE && index_buffer.handle != VK_NULL_HANDLE &&
        index_count > 0 && mesh && arrlen(mesh) > 0)
    {
        VkBuffer vertex_buffers[] = {position_buffer.handle, normal_buffer.handle,
                                     uv_buffer.handle};
        VkDeviceSize offsets[] = {0, 0, 0};
        vkCmdBindVertexBuffers(command->buffer, 0, 3, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command->buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

        Mesh* loaded_mesh = &mesh[0];
        const u32 instance_count =
            loaded_mesh->instances ? (u32)arrlen(loaded_mesh->instances) : 0;
        for (u32 i = 0; i < instance_count; ++i)
        {
            const MeshInstance& instance = loaded_mesh->instances[i];
            const MeshRange& range = loaded_mesh->ranges[instance.range_index];

            const u32 invalid_tex_index = 255;
            const u32 tex_mask = 0xffu;
            DrawPushConstants push = {};
            push.draw_id = i;
            push.material_index = range.material_index;
            if (loaded_mesh->materials && range.material_index < loaded_mesh->material_count)
            {
                const MaterialInfo& mat = loaded_mesh->materials[range.material_index];
                const u32 base_color =
                    mat.base_color_texture_index > tex_mask ? invalid_tex_index
                                                            : mat.base_color_texture_index;
                const u32 normal =
                    mat.normal_texture_index > tex_mask ? invalid_tex_index
                                                        : mat.normal_texture_index;
                const u32 metal_rough =
                    mat.metallic_roughness_texture_index > tex_mask
                        ? invalid_tex_index
                        : mat.metallic_roughness_texture_index;
                const u32 occlusion =
                    mat.occlusion_texture_index > tex_mask ? invalid_tex_index
                                                           : mat.occlusion_texture_index;
                push.packed_material =
                    ((base_color & tex_mask) << 0) | ((normal & tex_mask) << 8) |
                    ((metal_rough & tex_mask) << 16) | ((occlusion & tex_mask) << 24);
            }
            else
            {
                push.packed_material = 0xffffffffu;
            }
            vkCmdPushConstants(command->buffer, main_pipeline_layout->handle,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(DrawPushConstants), &push);
            vkCmdDrawIndexed(command->buffer, range.index_count, 1, range.index_offset,
                             (i32)range.vertex_offset, 0);
        }
    }
    else
    {
        vkCmdDraw(command->buffer, 3, 1, 0, 0);
    }
}

void VulkanRenderer::lightingPass()
{
    Command* command = &cmds[context.current_frame];
    RenderTarget* swapchain_rt = swapchain->render_targets[context.current_frame];

    // Transition GBuffer from render target to shader resource for sampling.
    RenderTargetBarrier gbuffer_barriers[GBUFFER_COUNT] = {};
    for (u32 i = 0; i < GBUFFER_COUNT; ++i)
    {
        gbuffer_barriers[i].render_target = gbuffer_render_targets[i];
        gbuffer_barriers[i].current_state = RESOURCE_STATE_RENDER_TARGET;
        gbuffer_barriers[i].new_state = RESOURCE_STATE_SHADER_RESOURCE;
    }

    // Ensure swapchain is in render-target state.
    TextureBarrier swapchain_barrier{};
    swapchain_barrier.current_state = RESOURCE_STATE_PRESENT;
    swapchain_barrier.new_state = RESOURCE_STATE_RENDER_TARGET;
    swapchain_barrier.texture = swapchain_rt->texture;

    vulkan_command_resource_barrier(command, NULL, 0, &swapchain_barrier, 1,
                                    gbuffer_barriers, GBUFFER_COUNT);

    RenderTarget* color_targets = swapchain_rt;
    RenderTargetOperator rendertarget_ops[2] = {};
    rendertarget_ops[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rendertarget_ops[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    RenderDesc render_desc{};
    render_desc.render_targets = &color_targets;
    render_desc.render_target_count = 1;
    render_desc.clear_color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    render_desc.render_area = {{0, 0}, {app_state_->width, app_state_->height}};
    render_desc.render_target_operators = rendertarget_ops;
    render_desc.depth_target = NULL;
    render_desc.is_depth_stencil = false;

    vulkan_command_buffer_rendering(command, &render_desc);

    VkViewport viewport = {0, 0, app_state_->width, app_state_->height, 0.f, 1.f};
    vkCmdSetViewport(command->buffer, 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, {app_state_->width, app_state_->height}};
    vkCmdSetScissor(command->buffer, 0, 1, &scissor);

    if (gbuffer_resolve_pipeline && gbuffer_resolve_layout)
    {
        vkCmdBindPipeline(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          gbuffer_resolve_pipeline->handle);
        if (gbuffer_desc_set != VK_NULL_HANDLE)
        {
            vkCmdBindDescriptorSets(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    gbuffer_resolve_layout->handle, 0, 1, &gbuffer_desc_set, 0,
                                    NULL);
        }
        vkCmdDraw(command->buffer, 3, 1, 0, 0);
    }

    // Transition GBuffer back to render-target for next frame.
    for (u32 i = 0; i < GBUFFER_COUNT; ++i)
    {
        gbuffer_barriers[i].current_state = RESOURCE_STATE_SHADER_RESOURCE;
        gbuffer_barriers[i].new_state = RESOURCE_STATE_RENDER_TARGET;
    }
    vulkan_command_resource_barrier(command, NULL, 0, NULL, 0, gbuffer_barriers, GBUFFER_COUNT);
}

void VulkanRenderer::debugPass()
{
    Command* command = &cmds[context.current_frame];
    RenderTarget* rendertarget = swapchain->render_targets[context.current_frame];

    if (use_gbuffer)
    {
        // Ensure swapchain image is in render-target state for imgui overlay.
        TextureBarrier textureBarrier{};
        textureBarrier.current_state = RESOURCE_STATE_PRESENT;
        textureBarrier.new_state = RESOURCE_STATE_RENDER_TARGET;
        textureBarrier.texture = rendertarget->texture;
        vulkan_command_resource_barrier(command, NULL, 0, &textureBarrier, 1, NULL, 0);
    }

    RenderTarget* rendertargets = rendertarget;
    RenderTargetOperator rendertarget_ops[2];
    rendertarget_ops[0].load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    rendertarget_ops[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    rendertarget_ops[1].load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    rendertarget_ops[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    RenderDesc render_desc{};
    render_desc.render_targets = &rendertargets;
    render_desc.render_target_count = 1;
    render_desc.render_area = {{0, 0}, {app_state_->width, app_state_->height}};
    render_desc.render_target_operators = rendertarget_ops;
    render_desc.depth_target = NULL;
    render_desc.is_depth_stencil = false;

    vulkan_command_buffer_rendering(command, &render_desc);
    drawImgui();
    vulkan_command_buffer_rendering(command, NULL);
}

void VulkanRenderer::endFrame()
{
    Command* command = &cmds[context.current_frame];
    RenderTarget* rendertarget = swapchain->render_targets[context.current_frame];

    // RenderTarget to Present
    TextureBarrier textureBarrier{};
    textureBarrier.current_state = RESOURCE_STATE_RENDER_TARGET;
    textureBarrier.new_state = RESOURCE_STATE_PRESENT;
    textureBarrier.texture = rendertarget->texture;

    vulkan_command_resource_barrier(command, NULL, 0, &textureBarrier, 1, NULL, 0);

    vulkan_command_buffer_end(command);

    VkPipelineStageFlags wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[context.current_frame];
    submit_info.pWaitDstStageMask = &wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command->buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &ready_to_render_semaphores[context.current_frame];

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info,
                           render_fences[context.current_frame]));

    if (!present_image_swapchain(&context, swapchain, context.device_context.present_queue,
                                 ready_to_render_semaphores[context.current_frame],
                                 context.image_index))
    {
        vkDeviceWaitIdle(context.device_context.handle);
        vulkan_swapchain_recreate(&context, &swapchain);
    }
}

void VulkanRenderer::Draw()
{
    if (!beginFrame())
    {
        return;
    }
    basePass();
    if (use_gbuffer)
    {
        lightingPass();
    }
    debugPass();
    endFrame();
}

void drawImgui()
{
    Command* command = &cmds[context.current_frame];

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool demo = true;
    ImGui::ShowDemoWindow(&demo);
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command->buffer);
}

void VulkanRenderer::Shutdown()
{
    vkQueueWaitIdle(context.device_context.graphics_queue);
    vkDeviceWaitIdle(context.device_context.handle);

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        vkDestroySemaphore(context.device_context.handle, image_available_semaphores[i],
                           context.allocator);
        vkDestroySemaphore(context.device_context.handle, ready_to_render_semaphores[i],
                           context.allocator);
        vkDestroyFence(context.device_context.handle, render_fences[i], context.allocator);

        image_available_semaphores[i] = VK_NULL_HANDLE;
        ready_to_render_semaphores[i] = VK_NULL_HANDLE;
        render_fences[i] = VK_NULL_HANDLE;
    }

    vulkan_swapchain_destroy(&context, swapchain);

    destroyMeshResources();

    // destroy imgui related objects
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(context.device_context.handle, context.imgui_pool, context.allocator);

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        vulkan_command_pool_destroy(&context, &cmds[i]);
    }

    destroyGBufferDescriptors();
    destroySceneDescriptors();
    destroyRenderTarget();
    destroyPipeline();
    destroyShader();
    destroyBuffer();

    vulkan_memory_allocator_destroy(&context);
    vulkan_device_destroy(&context, &context.device_context);

    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

#if defined(_DEBUG)
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, nullptr);
#endif
    // destroy instance
    vkDestroyInstance(context.instance, context.allocator);
}

b8 VulkanRenderer::createInstance()
{
    std::cout << "create instance" << std::endl;
    // TODO: custom allocator (for future use)
    context.allocator = nullptr;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "pko-engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "PKO_ENGINE";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo inst_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    inst_create_info.pApplicationInfo = &app_info;

    const char* required_layer_names[] = {"VK_LAYER_KHRONOS_validation"};

    u32 required_layer_count = sizeof(required_layer_names) / sizeof(const char*);

    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_properties(available_layer_count);
    VK_CHECK(
        vkEnumerateInstanceLayerProperties(&available_layer_count, available_properties.data()));

    for (u32 i = 0; i < required_layer_count; ++i)
    {
        std::cout << "Searching validation layer: " << required_layer_names[i] << "..."
                  << std::endl;
        b8 is_found = false;

        for (u32 l = 0; l < available_layer_count; ++l)
        {
            if (strcmp(required_layer_names[i], available_properties.at(l).layerName) == 0)
            {
                std::cout << "VULKAN INSTANCE LAYER: " << required_layer_names[i] << " found"
                          << std::endl;
                is_found = true;
                break;
            }
        }

        if (!is_found)
        {
            std::cout << "VULKAN INSTANCE LAYER: " << required_layer_names[i] << "is missing!\n";
            return false;
        }
    }

    inst_create_info.enabledLayerCount = required_layer_count;
    inst_create_info.ppEnabledLayerNames = required_layer_names;

    // TODO: this should be formatted as a function of inside the platform
    const char* required_extension_names[] = {VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
                                              // VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                                              "VK_KHR_win32_surface",
#elif defined(PKO_PLATFORM_LINUX)
		// VK_KHR_XCB_SURFACE_EXTENSION_NAME
		"VK_KHR_xcb_surface",
#else
    // UNSUPPORTED_PLATFORM
#endif  // PLATFORM DEPENDENT SURFACE EXTENSION NAME

#if defined(_DEBUG)
                                              VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif  //_DEBUG
    };

    inst_create_info.enabledExtensionCount =
        (u32)(sizeof(required_extension_names) / sizeof(const char*));
    inst_create_info.ppEnabledExtensionNames = required_extension_names;

    VK_CHECK(vkCreateInstance(&inst_create_info, context.allocator, &context.instance));

    return true;
}

void VulkanRenderer::createDebugUtilMessage()
{
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
    debugCreateInfo.pUserData = 0;

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debugCreateInfo, nullptr,
                                            &context.debug_messenger));
}

b8 VulkanRenderer::createSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};

    const InternalState& internal_state = app_state_->platform_state->GetInternalState();

    createInfo.hinstance = internal_state.instance;
    createInfo.hwnd = internal_state.handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocator,
                                     &context.surface));
    return true;
#endif
    return false;
}

void VulkanRenderer::createShader()
{
    ShaderLoadDesc shader_load_desc{};
    shader_load_desc.names[SHADER_STAGE_VERTEX] = "mesh_basic.vert";
    shader_load_desc.names[SHADER_STAGE_FRAGMENT] = "mesh_basic.frag";

    // Shader Create
    vulkan_shader_create(&context, &main_shader, &shader_load_desc);

    ShaderLoadDesc gbuffer_shader_desc{};
    gbuffer_shader_desc.names[SHADER_STAGE_VERTEX] = "gbuffer_resolve.vert";
    gbuffer_shader_desc.names[SHADER_STAGE_FRAGMENT] = "gbuffer_resolve.frag";
    vulkan_shader_create(&context, &gbuffer_resolve_shader, &gbuffer_shader_desc);
}

void VulkanRenderer::createPipeline()
{
    // Pipeline Layout Create
    vulkan_pipeline_layout_create(&context, main_shader, &main_pipeline_layout);
    vulkan_pipeline_layout_create(&context, gbuffer_resolve_shader, &gbuffer_resolve_layout);

    // Pipeline Create
    RasterizeDesc rasterize_desc{};
    rasterize_desc.depth_clamp_enable = VK_FALSE;
    rasterize_desc.rasterizer_discard_enable = VK_FALSE;
    rasterize_desc.polygon_mode = VK_POLYGON_MODE_FILL;
    rasterize_desc.cull_mode = VK_CULL_MODE_BACK_BIT;
    rasterize_desc.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterize_desc.depth_bias_enable = VK_FALSE;
    rasterize_desc.depth_bias_constant_factor = 0;
    rasterize_desc.depth_bias_clamp = 0;
    rasterize_desc.depth_bias_slope_factor = 0;
    rasterize_desc.line_width = 0.0f;

    DepthStencilDesc depth_stencil_desc{};
    depth_stencil_desc.depth_test_enable = VK_TRUE;
    depth_stencil_desc.depth_write_enable = VK_TRUE;
    depth_stencil_desc.depth_bounds_test_enable = VK_FALSE;
    depth_stencil_desc.depth_compare_op = VK_COMPARE_OP_LESS;
    depth_stencil_desc.min_depth_bounds = 0.0f;
    depth_stencil_desc.max_depth_bounds = 1.0f;
    depth_stencil_desc.stencil_test_enable = VK_FALSE;
    // depth_stencil_desc.stencil_front_op_state;
    // depth_stencil_desc.stencil_back_op_state;

    VertexInputBinding input_bindings[3];
    input_bindings[0].binding = 0;  // position
    input_bindings[0].stride = sizeof(float) * 3;
    input_bindings[0].input_rate = VK_VERTEX_INPUT_RATE_VERTEX;

    input_bindings[1].binding = 1;  // normal
    input_bindings[1].stride = sizeof(float) * 3;
    input_bindings[1].input_rate = VK_VERTEX_INPUT_RATE_VERTEX;

    input_bindings[2].binding = 2;  // uv
    input_bindings[2].stride = sizeof(float) * 2;
    input_bindings[2].input_rate = VK_VERTEX_INPUT_RATE_VERTEX;

    VertexInputAttribute input_attributes[3];
    input_attributes[0].location = 0;  // position
    input_attributes[0].binding = 0;
    input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[0].offset = 0;

    input_attributes[1].location = 1;  // normal
    input_attributes[1].binding = 1;
    input_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[1].offset = 0;

    input_attributes[2].location = 2;  // uv
    input_attributes[2].binding = 2;
    input_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    input_attributes[2].offset = 0;

    PipelineInputDesc input_desc{};
    input_desc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_desc.is_primitive_restart = VK_FALSE;
    input_desc.bindings = input_bindings;
    input_desc.binding_count = 3;
    input_desc.attributes = input_attributes;
    input_desc.attribute_count = 3;

    ColorBlendMode blend_mode = COLOR_BLEND_OPAQUE;
    VkFormat swapchain_format = swapchain->render_targets[0]->vulkan_format;

    PipelineDesc pipeline_desc{};
    pipeline_desc.rasterize_desc = &rasterize_desc;
    pipeline_desc.depth_stencil_desc = &depth_stencil_desc;
    pipeline_desc.input_desc = &input_desc;
    pipeline_desc.layout = main_pipeline_layout;
    pipeline_desc.shader = main_shader;
    pipeline_desc.blend_modes = &blend_mode;
    pipeline_desc.color_attachment_count = 1;
    pipeline_desc.color_attachment_formats = &swapchain_format;
    pipeline_desc.depth_attachment_format = depth_render_target->vulkan_format;
    // pipeline_desc.stencil_attachment_format;

    vulkan_graphics_pipeline_create(&context, &pipeline_desc, &main_pipeline);

    VkFormat gbuffer_formats[GBUFFER_COUNT] = {
        gbuffer_render_targets[GBUFFER_ALBEDO]->vulkan_format,
        gbuffer_render_targets[GBUFFER_NORMAL]->vulkan_format,
        gbuffer_render_targets[GBUFFER_MR]->vulkan_format};

    pipeline_desc.color_attachment_count = GBUFFER_COUNT;
    pipeline_desc.color_attachment_formats = gbuffer_formats;

    vulkan_graphics_pipeline_create(&context, &pipeline_desc, &gbuffer_pipeline);

    // GBuffer resolve pipeline (fullscreen triangle, no depth)
    RasterizeDesc resolve_rasterize = rasterize_desc;
    resolve_rasterize.cull_mode = VK_CULL_MODE_NONE;

    DepthStencilDesc resolve_depth{};
    resolve_depth.depth_test_enable = VK_FALSE;
    resolve_depth.depth_write_enable = VK_FALSE;
    resolve_depth.depth_bounds_test_enable = VK_FALSE;
    resolve_depth.depth_compare_op = VK_COMPARE_OP_ALWAYS;
    resolve_depth.stencil_test_enable = VK_FALSE;

    PipelineInputDesc resolve_input{};
    resolve_input.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    resolve_input.is_primitive_restart = VK_FALSE;
    resolve_input.bindings = NULL;
    resolve_input.binding_count = 0;
    resolve_input.attributes = NULL;
    resolve_input.attribute_count = 0;

    PipelineDesc resolve_desc{};
    resolve_desc.rasterize_desc = &resolve_rasterize;
    resolve_desc.depth_stencil_desc = &resolve_depth;
    resolve_desc.input_desc = &resolve_input;
    resolve_desc.layout = gbuffer_resolve_layout;
    resolve_desc.shader = gbuffer_resolve_shader;
    resolve_desc.blend_modes = &blend_mode;
    resolve_desc.color_attachment_count = 1;
    resolve_desc.color_attachment_formats = &swapchain_format;
    resolve_desc.depth_attachment_format = VK_FORMAT_UNDEFINED;

    vulkan_graphics_pipeline_create(&context, &resolve_desc, &gbuffer_resolve_pipeline);
}

void VulkanRenderer::createRenderTarget()
{
    ClearValue depth_clear_value{};
    depth_clear_value.depth = 1.0f;

    RenderTargetDesc depth_rt_desc{};
    depth_rt_desc.width = app_state_->width;
    depth_rt_desc.height = app_state_->height;
    depth_rt_desc.mip_levels = 1;
    depth_rt_desc.sample_count = 1;
    depth_rt_desc.vulkan_format = VK_FORMAT_D32_SFLOAT;
    depth_rt_desc.clear_value = depth_clear_value;
    depth_rt_desc.start_state = RESOURCE_STATE_DEPTH_WRITE;
    depth_rt_desc.descriptor_type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    vulkan_rendertarget_create(&context, &depth_rt_desc, &depth_render_target);

    // GBuffer scaffolding (not wired into rendering yet)
    auto create_gbuffer_target = [&](RenderTarget** out_rt, VkFormat format, ClearValue clear)
    {
        RenderTargetDesc desc{};
        desc.width = app_state_->width;
        desc.height = app_state_->height;
        desc.mip_levels = 1;
        desc.sample_count = 1;
        desc.vulkan_format = format;
        desc.clear_value = clear;
        desc.start_state = RESOURCE_STATE_RENDER_TARGET;
        desc.descriptor_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        vulkan_rendertarget_create(&context, &desc, out_rt);
    };

    ClearValue albedo_clear{};
    albedo_clear.r = 0.0f;
    albedo_clear.g = 0.0f;
    albedo_clear.b = 0.0f;
    albedo_clear.a = 1.0f;

    ClearValue normal_clear{};
    normal_clear.r = 0.5f;
    normal_clear.g = 0.5f;
    normal_clear.b = 1.0f;
    normal_clear.a = 1.0f;

    ClearValue mr_clear{};
    mr_clear.r = 0.0f;
    mr_clear.g = 0.0f;
    mr_clear.b = 0.0f;
    mr_clear.a = 1.0f;

    create_gbuffer_target(&gbuffer_render_targets[GBUFFER_ALBEDO], VK_FORMAT_R8G8B8A8_UNORM,
                          albedo_clear);
    create_gbuffer_target(&gbuffer_render_targets[GBUFFER_NORMAL], VK_FORMAT_R16G16B16A16_SFLOAT,
                          normal_clear);
    create_gbuffer_target(&gbuffer_render_targets[GBUFFER_MR], VK_FORMAT_R8G8B8A8_UNORM,
                          mr_clear);
}

void VulkanRenderer::createBuffer()
{
    // Camera UBO Create
    u32 camera_ubo_size = sizeof(CameraUBO);
    vulkan_buffer_create(&context, camera_ubo_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VMA_MEMORY_USAGE_CPU_TO_GPU, 0, &camera_ubo);

    if (mesh && arrlen(mesh) > 0)
    {
        Mesh* loaded_mesh = &mesh[0];
        const u32 instance_count =
            loaded_mesh->instances ? (u32)arrlen(loaded_mesh->instances) : 0;
        if (instance_count > 0)
        {
            DrawData* draw_data = (DrawData*)malloc(sizeof(DrawData) * instance_count);
            for (u32 i = 0; i < instance_count; ++i)
            {
                const MeshInstance& instance = loaded_mesh->instances[i];
                const MeshRange& range = loaded_mesh->ranges[instance.range_index];
                draw_data[i].model = instance.model_matrix;
                draw_data[i].material_index = range.material_index;
                draw_data[i].pad0 = 0;
                draw_data[i].pad1 = 0;
                draw_data[i].pad2 = 0;
            }

            draw_count = instance_count;
            const u32 draw_data_size = sizeof(DrawData) * draw_count;
            vulkan_buffer_create(&context, draw_data_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VMA_MEMORY_USAGE_CPU_TO_GPU, 0, &draw_data_buffer);
            vulkan_buffer_upload(&context, &draw_data_buffer, draw_data, draw_data_size);
            free(draw_data);
        }
        else
        {
            draw_count = 0;
        }

    }
}

void VulkanRenderer::createSceneDescriptors()
{
    destroySceneDescriptors();

    if (!main_pipeline_layout || camera_ubo.handle == VK_NULL_HANDLE ||
        draw_data_buffer.handle == VK_NULL_HANDLE)
    {
        return;
    }

    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                          MAX_BINDLESS_TEXTURES},
                                         {VK_DESCRIPTOR_TYPE_SAMPLER, 1}};

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = 4;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = 1;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VK_CHECK(vkCreateDescriptorPool(context.device_context.handle, &pool_info, context.allocator,
                                    &scene_desc_pool));

    VkDescriptorSetLayout layout = main_pipeline_layout->set_layouts[0];
    if (layout == VK_NULL_HANDLE)
    {
        destroySceneDescriptors();
        return;
    }

    VkDescriptorSetAllocateInfo allocate_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate_info.descriptorPool = scene_desc_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VK_CHECK(
        vkAllocateDescriptorSets(context.device_context.handle, &allocate_info, &scene_desc_set));

    VkDescriptorBufferInfo camera_info{};
    camera_info.buffer = camera_ubo.handle;
    camera_info.offset = 0;
    camera_info.range = sizeof(CameraUBO);

    VkDescriptorBufferInfo draw_info{};
    draw_info.buffer = draw_data_buffer.handle;
    draw_info.offset = 0;
    draw_info.range = sizeof(DrawData) * draw_count;

    u32 texture_count = textures ? (u32)arrlen(textures) : 0;
    assert(texture_count <= MAX_BINDLESS_TEXTURES);
    std::vector<VkDescriptorImageInfo> image_infos(texture_count);
    for (u32 i = 0; i < texture_count; ++i)
    {
        image_infos[i].sampler = VK_NULL_HANDLE;
        image_infos[i].imageView = textures[i].srv_descriptor;
        image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (texture_sampler == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        VK_CHECK(vkCreateSampler(context.device_context.handle, &sampler_info, context.allocator,
                                 &texture_sampler));
    }

    VkDescriptorImageInfo sampler_desc{};
    sampler_desc.sampler = texture_sampler;

    VkWriteDescriptorSet writes[4] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = scene_desc_set;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &camera_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = scene_desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo = &draw_info;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = scene_desc_set;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[2].descriptorCount = texture_count;
    writes[2].pImageInfo = image_infos.data();

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = scene_desc_set;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[3].descriptorCount = 1;
    writes[3].pImageInfo = &sampler_desc;
    vkUpdateDescriptorSets(context.device_context.handle, 4, writes, 0, nullptr);
}

void VulkanRenderer::createGBufferDescriptors()
{
    if (!gbuffer_resolve_layout || gbuffer_resolve_layout->set_layouts[0] == VK_NULL_HANDLE)
    {
        return;
    }

    if (gbuffer_desc_pool != VK_NULL_HANDLE)
    {
        destroyGBufferDescriptors();
    }

    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1},
                                         {VK_DESCRIPTOR_TYPE_SAMPLER, 1}};

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = 1;

    VK_CHECK(vkCreateDescriptorPool(context.device_context.handle, &pool_info, context.allocator,
                                    &gbuffer_desc_pool));

    VkDescriptorSetLayout layout = gbuffer_resolve_layout->set_layouts[0];
    VkDescriptorSetAllocateInfo allocate_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate_info.descriptorPool = gbuffer_desc_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VK_CHECK(
        vkAllocateDescriptorSets(context.device_context.handle, &allocate_info, &gbuffer_desc_set));

    if (texture_sampler == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        VK_CHECK(vkCreateSampler(context.device_context.handle, &sampler_info, context.allocator,
                                 &texture_sampler));
    }

    VkDescriptorImageInfo image_info{};
    image_info.imageView = gbuffer_render_targets[GBUFFER_ALBEDO]->descriptor;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo sampler_info{};
    sampler_info.sampler = texture_sampler;

    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = gbuffer_desc_set;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &image_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = gbuffer_desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &sampler_info;

    vkUpdateDescriptorSets(context.device_context.handle, 2, writes, 0, nullptr);
}

void VulkanRenderer::destroyGBufferDescriptors()
{
    if (gbuffer_desc_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(context.device_context.handle, gbuffer_desc_pool, context.allocator);
        gbuffer_desc_pool = VK_NULL_HANDLE;
        gbuffer_desc_set = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyShader()
{
    if (main_shader)
    {
        vulkan_shader_destroy(&context, main_shader);
        main_shader = nullptr;
    }

    if (gbuffer_resolve_shader)
    {
        vulkan_shader_destroy(&context, gbuffer_resolve_shader);
        gbuffer_resolve_shader = nullptr;
    }
}

void VulkanRenderer::destroyPipeline()
{
    if (main_pipeline)
    {
        vulkan_graphics_pipeline_destroy(&context, main_pipeline);
        free(main_pipeline);
        main_pipeline = nullptr;
    }

    if (gbuffer_pipeline)
    {
        vulkan_graphics_pipeline_destroy(&context, gbuffer_pipeline);
        free(gbuffer_pipeline);
        gbuffer_pipeline = nullptr;
    }

    if (main_pipeline_layout)
    {
        vulkan_pipeline_layout_destroy(&context, main_pipeline_layout);
        free(main_pipeline_layout);
        main_pipeline_layout = nullptr;
    }

    if (gbuffer_resolve_layout)
    {
        vulkan_pipeline_layout_destroy(&context, gbuffer_resolve_layout);
        free(gbuffer_resolve_layout);
        gbuffer_resolve_layout = nullptr;
    }
}

void VulkanRenderer::destroyRenderTarget()
{
    if (depth_render_target)
    {
        vulkan_rendertarget_destroy(&context, depth_render_target);
        depth_render_target = nullptr;
    }

    for (u32 i = 0; i < GBUFFER_COUNT; ++i)
    {
        if (gbuffer_render_targets[i])
        {
            vulkan_rendertarget_destroy(&context, gbuffer_render_targets[i]);
            gbuffer_render_targets[i] = nullptr;
        }
    }
}

void VulkanRenderer::destroyBuffer()
{
    if (camera_ubo.handle != VK_NULL_HANDLE)
    {
        vulkan_buffer_destroy(&context, &camera_ubo);
        camera_ubo.handle = VK_NULL_HANDLE;
        camera_ubo.allocation = nullptr;
    }

    if (draw_data_buffer.handle != VK_NULL_HANDLE)
    {
        vulkan_buffer_destroy(&context, &draw_data_buffer);
        draw_data_buffer.handle = VK_NULL_HANDLE;
        draw_data_buffer.allocation = nullptr;
    }

    draw_count = 0;

}

void VulkanRenderer::destroySceneDescriptors()
{
    if (scene_desc_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(context.device_context.handle, scene_desc_pool, context.allocator);
        scene_desc_pool = VK_NULL_HANDLE;
        scene_desc_set = VK_NULL_HANDLE;
    }

    if (texture_sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(context.device_context.handle, texture_sampler, context.allocator);
        texture_sampler = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyMeshResources()
{
    const auto destroy_buffer = [&](Buffer* buffer)
    {
        if (buffer && buffer->handle != VK_NULL_HANDLE)
        {
            vulkan_buffer_destroy(&context, buffer);
            buffer->handle = VK_NULL_HANDLE;
            buffer->allocation = nullptr;
        }
    };

    destroy_buffer(&position_buffer);
    destroy_buffer(&normal_buffer);
    destroy_buffer(&uv_buffer);
    destroy_buffer(&index_buffer);

    vertex_count = 0;
    index_count = 0;

    if (textures)
    {
        const u32 texture_count = (u32)arrlen(textures);
        for (u32 i = 0; i < texture_count; ++i)
        {
            vulkan_texture_destroy(&context, &textures[i]);
        }
        arrfree(textures);
        textures = NULL;
    }

    if (mesh)
    {
        free_mesh(mesh, 1);
        mesh = NULL;
    }
}

void VulkanRenderer::initImgui()
{
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(context.device_context.handle, &pool_info, nullptr,
                                    &context.imgui_pool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    const InternalState& state = app_state_->platform_state->GetInternalState();

    // this initializes imgui for SDL
    ImGui_ImplVulkan_LoadFunctions(
        [](const char* function_name, void*)
        { return vkGetInstanceProcAddr(context.instance, function_name); });
    ImGui_ImplWin32_Init(state.handle);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context.instance;
    init_info.PhysicalDevice = context.device_context.physical_device;
    init_info.Device = context.device_context.handle;
    init_info.Queue = context.device_context.graphics_queue;
    init_info.DescriptorPool = context.imgui_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
    init_info.ColorAttachmentFormat = swapchain->surface_format.format;
    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    // execute a gpu command to upload imgui font textures

    // Begin Command
    Command oneTimeSubmit;
    vulkan_command_pool_create(&context, &oneTimeSubmit, QUEUE_TYPE_GRAPHICS);
    vulkan_command_buffer_allocate(&context, &oneTimeSubmit, true);
    vulkan_command_buffer_begin(&oneTimeSubmit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    ImGui_ImplVulkan_CreateFontsTexture();
    vulkan_command_buffer_end(&oneTimeSubmit);

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &oneTimeSubmit.buffer;

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(context.device_context.graphics_queue);

    vulkan_command_pool_destroy(&context, &oneTimeSubmit);
}

b8 load_exported_entry_points()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define LoadProcAddress GetProcAddress
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
#define LoadProcAddress dlsym
#endif
#define VK_EXPORTED_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)LoadProcAddress(vulkan_library_loader, #fun)))              \
    {                                                                                  \
        std::cout << "Could not load exported function: " << #fun << "!" << std::endl; \
        return false;                                                                  \
    }

#include "list_of_functions.inl"

    return true;
}

b8 load_global_level_function()
{
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun)))                          \
    {                                                                                      \
        std::cout << "Could not load global level function: " << #fun << "!" << std::endl; \
        return false;                                                                      \
    }

#include "list_of_functions.inl"

    return true;
}

b8 load_instance_level_function()
{
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                                    \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(context.instance, #fun)))                 \
    {                                                                                      \
        std::cout << "Could not load global level function: " << #fun << "!" << std::endl; \
        return false;                                                                      \
    }

#include "list_of_functions.inl"

    return true;
}

b8 load_device_level_function()
{
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(context.device_context.handle, #fun)))      \
    {                                                                                      \
        std::cout << "Could not load Device level function: " << #fun << "!" << std::endl; \
        return false;                                                                      \
    }

#include "list_of_functions.inl"
    return true;
}
