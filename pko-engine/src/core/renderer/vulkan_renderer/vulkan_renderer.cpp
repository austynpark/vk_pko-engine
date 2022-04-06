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

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

static vulkan_library vulkan_library_loader;
static vulkan_context context;

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

vulkan_renderer::vulkan_renderer(void* platform_internal_state) : renderer(platform_internal_state)
{
}

vulkan_renderer::~vulkan_renderer()
{
    shutdown();
#if defined _WIN32
    FreeLibrary(vulkan_library_loader);
#elif defined _linux
#endif

    vulkan_library_loader = 0;
}

b8 vulkan_renderer::init()
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

    if (!create_instance()) {
        std::cout << "create instance failed" << std::endl;
        return false;
    }

    if (!load_instance_level_function())
        return false;

    //TODO: not using this (instead will use event_system)
    get_app_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);

    if (!create_surface()) {
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
    create_debug_util_message();
#endif 

    if (!vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain)) {
        std::cout << "create swapchain failed" << std::endl;
        return false;
    }
    
    // create command pool per frame
	context.graphics_commands.resize(context.swapchain.max_frames_in_flight);
    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
		vulkan_command_pool_create(&context, &context.graphics_commands.at(i), context.device_context.graphics_family.index);
		vulkan_command_buffer_allocate(&context, &context.graphics_commands.at(i), true);
    }

    vulkan_renderpass_create(&context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height);
    
    context.framebuffers.resize(context.swapchain.image_count);

    for (u32 i = 0; i < context.swapchain.image_count; ++i) {

        VkImageView image_views[] = {
            context.swapchain.image_views.at(i),
            context.swapchain.depth_attachment.view
        };

        u32 attachment_count = 2;

       vulkan_framebuffer_create(&context, &context.main_renderpass, attachment_count, image_views, &context.framebuffers.at(i));
    }

    std::cout << "framebuffers created" << std::endl;

    context.image_available_semaphores.resize(context.swapchain.max_frames_in_flight);
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

    global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32)context.framebuffer_width / context.framebuffer_height, 0.1f, 100.0f);
    // camera pos, pos + front, up
    global_ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(global_ubo);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    if (!vulkan_graphics_pipeline_create(
        &context, &context.main_renderpass,
        &context.graphics_pipeline,
        1,
        &binding_description,
        3,
        attribute_descriptions,
        1,
        &push_constant_range,
        "shader/test.vert.spv", "shader/test.frag.spv")) {
        std::cout << "graphics pipeline create failed\n";

        return false;
    }

    for (const auto& obj : object_manager) {
        vulkan_render_object* vk_render_obj = (vulkan_render_object*)(obj.second.get());
        vk_render_obj->upload_mesh(&context);
    }

	return true;
}

b8 vulkan_renderer::draw()
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
        &context.image_index))
    {
        std::cout << "image acquire failed" << std::endl;
		return false;
    }

    //TODO: multiple command buffers
    VkCommandBuffer command_buffer = context.graphics_commands.at(context.current_frame).buffers.at(0);

    VK_CHECK(vkResetCommandBuffer(command_buffer, 0));
    VkCommandBufferBeginInfo cmd_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &cmd_begin_info));

    VkClearValue clear_values[] = {
        // color
        { {0.3f, 0.2f, 0.1f, 1.0f} },
        // depth, stencil
        {{1.0f, 0.0f}}
    };

    VkRenderPassBeginInfo renderpass_begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderpass_begin_info.renderPass = context.main_renderpass.handle;
    renderpass_begin_info.framebuffer = context.framebuffers.at(context.image_index);
    renderpass_begin_info.renderArea.extent = { context.main_renderpass.width, context.main_renderpass.height };
    renderpass_begin_info.renderArea.offset.x = context.main_renderpass.x;
    renderpass_begin_info.renderArea.offset.y = context.main_renderpass.y;
    renderpass_begin_info.clearValueCount = 2;
    renderpass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vulkan_pipeline_bind(&context.graphics_commands.at(context.current_frame), VK_PIPELINE_BIND_POINT_GRAPHICS, &context.graphics_pipeline);
    
	vkCmdPushConstants(command_buffer, context.graphics_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(global_ubo), &global_ubo);

    for (const auto& obj : object_manager) {
        VkDeviceSize offset = 0;
        vulkan_render_object* vk_render_obj = (vulkan_render_object*)(obj.second.get());
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vk_render_obj->vertex_buffer.buffer, &offset);
        vkCmdDraw(command_buffer, vk_render_obj->p_mesh->vertices.size(), 1, 0, 0);
    }

    vkCmdEndRenderPass(command_buffer);

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
    present_image_swapchain(&context,
        &context.swapchain,
        context.device_context.present_queue,
        context.ready_to_render_semaphores.at(context.current_frame),
        context.image_index);

    ++frame_number;

	return true;
}

void vulkan_renderer::shutdown()
{
    vkQueueWaitIdle(context.device_context.graphics_queue);

    for (const auto& obj : object_manager) {
        vulkan_render_object* vk_render_obj = (vulkan_render_object*)(obj.second.get());
        vk_render_obj->vulkan_render_object_destroy(&context);
    }

    vulkan_pipeline_destroy(&context, &context.graphics_pipeline);


    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        vkDestroySemaphore(context.device_context.handle, context.image_available_semaphores.at(i), context.allocator);
        vkDestroySemaphore(context.device_context.handle, context.ready_to_render_semaphores.at(i), context.allocator);
		vkDestroyFence(context.device_context.handle, context.render_fences.at(i), context.allocator);
    }
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(&context, &context.framebuffers.at(i));
    }

    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
        vulkan_command_pool_destroy(&context, &context.graphics_commands.at(i));
    vulkan_swapchain_destroy(&context, &context.swapchain);
    vulkan_memory_allocator_destroy(&context);
    vulkan_device_destroy(&context, &context.device_context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);
}

b8 vulkan_renderer::on_resize(u32 w, u32 h)
{
    std::cout << "resize " << w << h << std::endl;

	return true;
}

b8 vulkan_renderer::create_instance()
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

void vulkan_renderer::create_debug_util_message()
{
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
    debugCreateInfo.pUserData = this;

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debugCreateInfo, nullptr, &context.debug_messenger));
}

b8 vulkan_renderer::create_surface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };

    createInfo.hinstance = ((internal_state*)plat_state)->instance;
    createInfo.hwnd = ((internal_state*)plat_state)->handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocator, &context.surface));
    return true;
#endif
    return false;
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