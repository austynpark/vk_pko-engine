#include "vulkan_renderer.h"

#define VMA_IMPLEMENTATION
#include "vulkan_types.inl"

#include "core/application.h"
#include "platform/platform.h"

#include "vulkan_device.h"
#include "vulkan_memory_allocate.h"
#include "vulkan_swapchain.h"
#include "vulkan_command_buffer.h"
#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_mesh.h"
#include "vulkan_buffer.h"
#include "vulkan_shader.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <iostream>

static uint32_t MAX_FRAME = 3;

static vulkan_library vulkan_library_loader;
static VulkanContext context;

VkSemaphore ready_to_render_semaphores[MAX_FRAME];
VkSemaphore image_available_semaphores[MAX_FRAME];
VkFence render_fences[MAX_FRAME];

static VKAPI_ATTR VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    switch (messageSeverity) {
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

VulkanRenderer::VulkanRenderer(void* platform_internal_state) : Renderer(platform_internal_state)
{
}

VulkanRenderer::~VulkanRenderer()
{
    Shutdown();
#if defined _WIN32
    FreeLibrary(vulkan_library_loader);
#elif defined _linux
#endif

    vulkan_library_loader = 0;
}

b8 VulkanRenderer::Init()
{
#if defined(_WIN32)
	vulkan_library_loader = LoadLibrary("vulkan-1.dll");
#endif

	if (vulkan_library_loader == 0) {
		std::cout << "vulkan library failed to load" << std::endl;
		return false;
	}

	if (!load_exported_entry_points())
		return false;

    if (!load_global_level_function())
        return false;

    if (!createInstance()) {
        std::cout << "create instance failed" << std::endl;
        return false;
    }

    if (!load_instance_level_function())
        return false;

    if (!createSurface()) {
        std::cout << "create surface failed" << std::endl;
        return false;
    }

    if (!vulkan_device_create(&context, &context.device_context)) {
        std::cout << "create device failed" << std::endl;
        return false;
    }

    if (!load_device_level_function())
        return false;

    vulkan_memory_allocator_create(&context);

    vulkan_get_device_queue(&context.device_context);

	    //validation debug logger create
#if defined(_DEBUG)
    createDebugUtilMessage();
#endif 

    if (!vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain)) {
        std::cout << "create swapchain failed" << std::endl;
        return false;
    }
    
    initImgui();

    image_available_semaphores.resize(context.swapchain.max_frames_in_flight);
    context.ready_to_render_semaphores.resize(context.swapchain.max_frames_in_flight);
    context.render_fences.resize(context.swapchain.max_frames_in_flight);

    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &context.image_available_semaphores.at(i)));
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &context.ready_to_render_semaphores.at(i)));
        VkFenceCreateInfo fence_create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(context.device_context.handle, &fence_create_info, context.allocator, &context.render_fences.at(i));
    }
    std::cout << "sync objects created" << std::endl;

	// descriptor allocator init
    context.dynamic_descriptor_allocators.resize(MAX_FRAME);
    for (auto& desc_alloc : context.dynamic_descriptor_allocators) {
        desc_alloc.init(context.device_context.handle);
    }

    /*
    * global descriptor initialize
    */
    //vulkan_global_data_initialize(&context, sizeof(global_ubo));

    /*

    //TODO: move to shader
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = sizeof(vertex);

    VkVertexInputAttributeDescription position_attribute{};
    position_attribute.binding = 0;
    position_attribute.location = 0;
    position_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription normal_attribute{};
    normal_attribute.binding = 0;
    normal_attribute.location = 1;
    normal_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription uv_attribute{};
    uv_attribute.binding = 0;
    uv_attribute.location = 2;
    uv_attribute.format = VK_FORMAT_R32G32_SFLOAT;

    VkVertexInputAttributeDescription attribute_descriptions[] = {
        position_attribute,
        normal_attribute,
        uv_attribute
    };

    /*
    global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32)context.framebuffer_width / context.framebuffer_height, 0.1f, 100.0f);
    // camera pos, pos + front, up
    global_ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    // Init global data
    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(global_ubo);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    */

    /*
    if (!vulkan_graphics_pipeline_create(
        &context, &context.main_renderpass,
        &context.graphics_pipeline,
        1,
        &binding_description,
        3,
        attribute_descriptions,
        0,
        nullptr,
        1,
        &context.global_data.set_layout,
        "shader/test.vert.spv", "shader/test.frag.spv")) {
        std::cout << "graphics pipeline create failed\n";

        return false;
    }
    */
    
	return true;
}

void VulkanRenderer::Load()
{

}

void VulkanRenderer::UnLoad()
{

}

void VulkanRenderer::Update(float deltaTime)
{

}

void VulkanRenderer::Draw()
{

}

b8 VulkanRenderer::begin_frame(f32 dt)
{
    context.current_frame = frame_number % context.swapchain.max_frames_in_flight;

	VK_CHECK(vkWaitForFences(context.device_context.handle, 1, &context.render_fences.at(context.current_frame), true, UINT64_MAX));
    VK_CHECK(vkResetFences(context.device_context.handle, 1, &context.render_fences.at(context.current_frame)));

    if (!acquire_next_image_index_swapchain(
        &context,
        &context.swapchain,
        UINT64_MAX,
        context.image_available_semaphores.at(context.current_frame),
        0,
        &scene[scene_index]->image_index))
    {
        std::cout << "image acquire failed" << std::endl;
        return false;
    }

    //TODO: multiple command buffers
    VkCommandBuffer command_buffer = scene[scene_index]->graphics_commands.at(context.current_frame).buffer;

    VK_CHECK(vkResetCommandBuffer(command_buffer, 0));
    VkCommandBufferBeginInfo cmd_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &cmd_begin_info));

    return scene[scene_index]->begin_frame(dt);
}

b8 VulkanRenderer::end_frame()
{
	//TODO: multiple command buffers
    VkCommandBuffer command_buffer = scene[scene_index]->graphics_commands.at(context.current_frame).buffer;

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkPipelineStageFlags wait_stages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores.at(context.current_frame);
    submit_info.pWaitDstStageMask = &wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.ready_to_render_semaphores.at(context.current_frame);

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info, context.render_fences.at(context.current_frame)));

    /*
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &context.ready_to_render_semaphores.at(0);
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context.swapchain.handle;
    present_info.pImageIndices = &context.image_index;

    VK_CHECK(vkQueuePresentKHR(context.device_context.graphics_queue, &present_info));
    */
    if (!present_image_swapchain(&context,
        &context.swapchain,
        context.device_context.present_queue,
        context.ready_to_render_semaphores.at(context.current_frame),
        scene[scene_index]->image_index)) {

        /*
        get_app_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);
        vulkan_swapchain_recreate(&context, context.framebuffer_width, context.framebuffer_height);
        regenerate_framebuffer();
        */
    }


    return scene[scene_index]->end_frame();
}

void VulkanRenderer::Draw()
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command.buffer);
}

//b8 VulkanRenderer::drawImgui()
//{
//    ImGui_ImplVulkan_NewFrame();
//    ImGui_ImplWin32_NewFrame();
//    ImGui::NewFrame();
//
//    bool demo = true;
//    ImGui::ShowDemoWindow(&demo);
//    ImGuiIO& io = ImGui::GetIO();
//    ImGui::Render();
//
//
//    return true;
//}

void VulkanRenderer::Shutdown()
{
    vkQueueWaitIdle(context.device_context.graphics_queue);

    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        vkDestroySemaphore(context.device_context.handle, context.image_available_semaphores.at(i), context.allocator);
        vkDestroySemaphore(context.device_context.handle, context.ready_to_render_semaphores.at(i), context.allocator);
        vkDestroyFence(context.device_context.handle, context.render_fences.at(i), context.allocator);

        context.image_available_semaphores.at(i) = VK_NULL_HANDLE;
        context.ready_to_render_semaphores.at(i) = VK_NULL_HANDLE;
        context.render_fences.at(i) = VK_NULL_HANDLE;
    }

    vulkan_global_data_destroy(&context);

    scene[scene_index]->shutdown();
	vulkan_renderpass_destroy(&context, &context.main_renderpass);

    vulkan_swapchain_destroy(&context, &context.swapchain);

    // destroy imgui related objects
	vkDestroyDescriptorPool(context.device_context.handle, context.imgui_pool, context.allocator);
	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	ImGui_ImplVulkan_Shutdown();

    vulkan_memory_allocator_destroy(&context);
    vulkan_device_destroy(&context, &context.device_context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);
}

//b8 VulkanRenderer::onResize(u32 w, u32 h)
//{
//    if (context.device_context.handle != nullptr) {
//        vkDeviceWaitIdle(context.device_context.handle);
//
//		//get_app_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);
//        context.framebuffer_width = w;
//        context.framebuffer_height = h;
//
//        if (!vulkan_swapchain_recreate(&context, context.framebuffer_width, context.framebuffer_height))
//            return false;
//
//        scene[scene_index]->on_resize(w, h);
//
//        return true;
//    }
//
//	return false;
//}

b8 VulkanRenderer::createInstance()
{
    std::cout << "create instance" << std::endl;
    //TODO: custom allocator (for future use)
    context.allocator = nullptr;
   
    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = "pko-engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "PKO_ENGINE";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo inst_create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    inst_create_info.pApplicationInfo = &app_info;

#if defined(_DEBUG)
    const char* required_layer_names[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    u32 required_layer_count = sizeof(required_layer_names) / sizeof(const char*);

    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_properties(available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_properties.data()));

    for (u32 i = 0; i < required_layer_count; ++i) {
        std:: cout << "Searching validation layer: " << required_layer_names[i] << "..." << std::endl;
        b8 is_found = false;;
        for (u32 l = 0; l < available_layer_count; ++l) {
            if (strcmp(required_layer_names[i], available_properties.at(l).layerName) == 0) {
                std::cout << "found" << std::endl;
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            std::cout << required_layer_names[i] << "is missing!\n";
            return false;
        }
    }

    inst_create_info.enabledLayerCount = required_layer_count;
    inst_create_info.ppEnabledLayerNames = required_layer_names;
#endif //_DEBUG

    //TODO: this should be formatted as a function of inside the platform
    const char* required_extension_names[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
        //VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        "VK_KHR_win32_surface",
#elif defined(PKO_PLATFORM_LINUX)
        //VK_KHR_XCB_SURFACE_EXTENSION_NAME
        "VK_KHR_xcb_surface",
#else
        //UNSUPPORTED_PLATFORM
#endif //PLATFORM DEPENDENT SURFACE EXTENSION NAME

#if defined(_DEBUG)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif //_DEBUG
    };

    inst_create_info.enabledExtensionCount = (u32)(sizeof(required_extension_names) / sizeof(const char*));
    inst_create_info.ppEnabledExtensionNames = required_extension_names;

    VK_CHECK(vkCreateInstance(&inst_create_info, context.allocator, &context.instance));

    return true;

}

void VulkanRenderer::createDebugUtilMessage()
{
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
    debugCreateInfo.pUserData = 0;

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debugCreateInfo, nullptr, &context.debug_messenger));
}

b8 VulkanRenderer::createSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    

    const InternalState* internal_state = (InternalState*)((PlatformState*)platform_state)->state;
    assert(internal_state);

    createInfo.hinstance = internal_state->instance;
    createInfo.hwnd = ((InternalState*)platform_state)->handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocator, &context.surface));
    return true;
#endif
    return false;
}

void VulkanRenderer::initImgui()
{
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(context.device_context.handle, &pool_info, nullptr, &context.imgui_pool));


    // 2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    InternalState* state = (InternalState*)platform_state;

    //this initializes imgui for SDL
    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*) { return vkGetInstanceProcAddr(context.instance, function_name); });
    ImGui_ImplWin32_Init(state->handle);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context.instance;
    init_info.PhysicalDevice = context.device_context.physical_device;
    init_info.Device = context.device_context.handle;
    init_info.Queue = context.device_context.graphics_queue;
    init_info.DescriptorPool = context.imgui_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, context.main_renderpass.handle);

    //execute a gpu command to upload imgui font textures

    // Begin Command
    VulkanCommand command = scene[scene_index]->graphics_commands.at(context.current_frame);

    vulkan_command_buffer_begin(&command, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	ImGui_ImplVulkan_CreateFontsTexture(command.buffer);
    vulkan_command_buffer_end(&command);

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command.buffer;

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
#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)LoadProcAddress( vulkan_library_loader, #fun )) ) {                \
      std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
      return false;                                                                   \
    }

#include "list_of_functions.inl"

	return true;
}

b8 load_global_level_function() {
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "list_of_functions.inl"

    return true;
}

b8 load_instance_level_function() {
#define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( context.instance, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "list_of_functions.inl"

    return true;
}

b8 load_device_level_function() {
#define VK_DEVICE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( context.device_context.handle, #fun )) ) {                    \
      std::cout << "Could not load Device level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "list_of_functions.inl"
    return true;
}

