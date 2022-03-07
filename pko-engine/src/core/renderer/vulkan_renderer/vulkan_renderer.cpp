#include "vulkan_renderer.h"
#include "core/application.h"
#include "platform/platform.h"
#include "vulkan_types.inl"

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_command_buffer.h"
#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"

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

    vulkan_get_device_queue(&context.device_context);

	    //validation debug logger create
#if defined(_DEBUG)
    create_debug_util_message();
#endif 

    if (!vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain)) {
        std::cout << "create swapchain failed" << std::endl;
        return false;
    }
    
    vulkan_command_pool_create(&context, &context.graphics_command, context.device_context.graphics_family.index);
    vulkan_command_buffer_allocate(&context, &context.graphics_command, true);


    vulkan_renderpass_create(&context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height);
    
    context.framebuffers.resize(context.swapchain.image_count);

    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
       vulkan_framebuffer_create(&context, &context.main_renderpass, &context.swapchain.image_views.at(i), &context.framebuffers.at(i));
    }

    std::cout << "framebuffers created" << std::endl;

    context.image_available_semaphores.resize(context.swapchain.image_count);
    context.ready_to_render_semaphores.resize(context.swapchain.image_count);

    VkSemaphoreCreateInfo semaphore_create_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &context.image_available_semaphores.at(0)));
    VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &context.ready_to_render_semaphores.at(0)));
    VkFenceCreateInfo fence_create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(context.device_context.handle, &fence_create_info, context.allocator, &context.fence);

	return true;
}

b8 vulkan_renderer::draw(f32 dt)
{
    VK_CHECK(vkWaitForFences(context.device_context.handle, 1, &context.fence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(context.device_context.handle, 1, &context.fence));

    if (!acquire_next_image_index_swapchain(
        &context,
        &context.swapchain,
        UINT64_MAX,
        context.image_available_semaphores.at(0),
        0,
        &context.image_index)
        ) 
    {
        std::cout << "image acquire failed" << std::endl;
		return false;
    }

    VkCommandBuffer command_buffer = context.graphics_command.buffer;

    VK_CHECK(vkResetCommandBuffer(command_buffer, 0));
    VkCommandBufferBeginInfo cmd_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &cmd_begin_info));

    VkClearValue clear_value{};
    clear_value.color = { {0.3f, 0.2f, 0.1f, 1.0f} };

    VkRenderPassBeginInfo renderpass_begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderpass_begin_info.renderPass = context.main_renderpass.handle;
    renderpass_begin_info.framebuffer = context.framebuffers.at(context.image_index);
    renderpass_begin_info.renderArea.extent = { context.main_renderpass.width, context.main_renderpass.height };
    renderpass_begin_info.renderArea.offset.x = context.main_renderpass.x;
    renderpass_begin_info.renderArea.offset.y = context.main_renderpass.y;
    renderpass_begin_info.clearValueCount = 1;
    renderpass_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer);

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkPipelineStageFlags wait_stages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores.at(0);
    submit_info.pWaitDstStageMask = &wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.ready_to_render_semaphores.at(0);

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info, context.fence));

    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &context.ready_to_render_semaphores.at(0);
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context.swapchain.handle;
    present_info.pImageIndices = &context.image_index;

    VK_CHECK(vkQueuePresentKHR(context.device_context.graphics_queue, &present_info));

    ++frame_number;

	return true;
}

void vulkan_renderer::shutdown()
{
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(&context, &context.framebuffers.at(i));
    }

    vulkan_renderpass_destroy(&context, &context.main_renderpass);
    vulkan_command_pool_destroy(&context, &context.graphics_command);
    vulkan_swapchain_destroy(&context, &context.swapchain);
    vulkan_device_destroy(&context, &context.device_context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);
}

b8 vulkan_renderer::on_resize(u32 w, u32 h)
{
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