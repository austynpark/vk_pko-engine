#include "vulkan_renderer.h"

#define VMA_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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

static vulkan_library vulkan_library_loader{};
static RenderContext context{};

Command cmds[MAX_FRAME];

VulkanSwapchain* swapchain = NULL;
VkSemaphore ready_to_render_semaphores[MAX_FRAME];
VkSemaphore image_available_semaphores[MAX_FRAME];
VkFence render_fences[MAX_FRAME];

RenderTarget* depth_render_target = NULL;
PipelineLayout* main_pipeline_layout = NULL;
Pipeline* main_pipeline = NULL;
Shader* main_shader = NULL;

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

    return true;
}

void VulkanRenderer::Load(ReloadDesc* desc)
{
    ReloadType reload_type = desc->type;

    if (reload_type & RELOAD_TYPE_RESIZE)
    {
        ShaderLoadDesc shader_load_desc{};
        shader_load_desc.names[SHADER_STAGE_VERTEX] = "test.vert";
        shader_load_desc.names[SHADER_STAGE_FRAGMENT] = "test.frag";

        // Shader Create
        vulkan_shader_create(&context, &main_shader, &shader_load_desc);
        // Pipeline Layout Create
        vulkan_pipeline_layout_create(&context, main_shader, &main_pipeline_layout);

        createRenderTarget();

        createPipeline();
    }
}

void VulkanRenderer::UnLoad(ReloadDesc* desc)
{
    ReloadType reload_type = desc->type;

    if (reload_type & RELOAD_TYPE_RESIZE)
    {
        vulkan_graphics_pipeline_destroy(&context, main_pipeline);
        vulkan_rendertarget_destroy(&context, depth_render_target);
    }
}

void VulkanRenderer::Update(float deltaTime) {}

void VulkanRenderer::Draw()
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
        return;
    }

    Command* command = &cmds[context.current_frame];
    RenderTarget* rendertarget = swapchain->render_targets[context.current_frame];

    vulkan_command_pool_reset(command);
    vulkan_command_buffer_begin(command, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Present to RenderTarget
    TextureBarrier textureBarrier{};
    textureBarrier.current_state = RESOURCE_STATE_PRESENT;
    textureBarrier.new_state = RESOURCE_STATE_RENDER_TARGET;
    textureBarrier.texture = rendertarget->texture;

    vulkan_command_resource_barrier(command, NULL, 0, &textureBarrier, 1, NULL, 0);

    RenderTarget* rendertargets = rendertarget;
    RenderTargetOperator rendertarget_ops[2];
    rendertarget_ops[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rendertarget_ops[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    rendertarget_ops[1].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rendertarget_ops[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    RenderDesc render_desc{};
    render_desc.render_targets = &rendertargets;
    render_desc.render_target_count = 1;
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
    vkCmdBindPipeline(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline->handle);
    vkCmdDraw(command->buffer, 3, 1, 0, 0);

    // rendering imgui
    render_desc.depth_target = NULL;
    rendertarget_ops[0].load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    rendertarget_ops[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    vulkan_command_buffer_rendering(command, &render_desc);

    drawImgui();

    vulkan_command_buffer_rendering(command, NULL);

    // RenderTarget to Present
    textureBarrier = {};
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

    // destroy imgui related objects
    vkDestroyDescriptorPool(context.device_context.handle, context.imgui_pool, context.allocator);
    ImGui_ImplVulkan_Shutdown();

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        vulkan_command_pool_destroy(&context, &cmds[i]);
    }

    vulkan_memory_allocator_destroy(&context);
    vulkan_device_destroy(&context, &context.device_context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);

#if defined _WIN32
    FreeLibrary(vulkan_library_loader);
#elif defined _linux
#endif

    vulkan_library_loader = 0;
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

void VulkanRenderer::createPipeline()
{
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

    VertexInputBinding input_binding;
    input_binding.binding = 0;
    input_binding.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
    input_binding.stride = sizeof(Vertex);

    VertexInputAttribute input_attributes[3];
    input_attributes[0].binding = 0;
    input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[0].location = 0;
    input_attributes[0].offset = offsetof(Vertex, position);

    input_attributes[1].binding = 0;
    input_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[1].location = 1;
    input_attributes[1].offset = offsetof(Vertex, normal);

    input_attributes[2].binding = 0;
    input_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    input_attributes[2].location = 2;
    input_attributes[2].offset = offsetof(Vertex, uv);

    PipelineInputDesc input_desc{};
    input_desc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_desc.is_primitive_restart = VK_FALSE;
    input_desc.bindings = NULL;  // &input_binding;
    input_desc.binding_count = 1;
    input_desc.attributes = NULL;  // input_attributes;
    input_desc.attribute_count = 3;

    ColorBlendMode blend_mode = COLOR_BLEND_OPAQUE;
    VkFormat color_attachment_format = swapchain->render_targets[0]->vulkan_format;

    PipelineDesc pipeline_desc{};
    pipeline_desc.rasterize_desc = &rasterize_desc;
    pipeline_desc.depth_stencil_desc = &depth_stencil_desc;
    pipeline_desc.input_desc = &input_desc;
    pipeline_desc.layout = main_pipeline_layout;
    pipeline_desc.shader = main_shader;
    pipeline_desc.blend_modes = &blend_mode;
    pipeline_desc.color_attachment_count = 1;
    pipeline_desc.color_attachment_formats = &color_attachment_format;
    pipeline_desc.depth_attachment_format = depth_render_target->vulkan_format;
    // pipeline_desc.stencil_attachment_format;

    vulkan_graphics_pipeline_create(&context, &pipeline_desc, &main_pipeline);
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
