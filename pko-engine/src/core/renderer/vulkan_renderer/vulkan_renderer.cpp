#include "vulkan_renderer.h"

#define VMA_IMPLEMENTATION
#include "vulkan_types.inl"

#include "core/application.h"
#include "core/file_handle.h"
#include "platform/platform.h"

#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_memory_allocate.h"
#include "vulkan_swapchain.h"
#include "vulkan_pipeline.h"
#include "vulkan_buffer.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <iostream>
#include <glm/glm.hpp>

static vulkan_library vulkan_library_loader = {};
static RenderContext context = {};
static internal_state* platform_state = nullptr;

u32 currentFrame = 0;
u32 frameCount = 0;

VkSemaphore ready_to_render_semaphores[MAX_FRAME];
VkSemaphore image_available_semaphores[MAX_FRAME];
VkFence render_fences[MAX_FRAME];

VkCommandPool cmdPools[MAX_FRAME];
VkCommandBuffer cmdBuffs[MAX_FRAME];

VkShaderModule basicVertShaderModule;
VkShaderModule basicFragShaderModule;
VkPipeline basicPipeline;

VkDescriptorSet updateFreqNoneDescriptorSet;
VkDescriptorSetLayout updateFreqNonedescriptorSetLayout;
 

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

b8 vulkan_shader_module_create(RenderContext* context, VkShaderModule* out_shaderModule, const char* path)
{
    file_handle file;
    if (!pko_file_read(path, &file)) {
        std::cout << "failed to read " << path << std::endl;
        return false;
    }
    VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    create_info.pCode = (uint32_t*)file.str;
    create_info.codeSize = file.size;

    VK_CHECK(vkCreateShaderModule(context->device_context.handle, &create_info, context->allocator, out_shaderModule));

    pko_file_close(&file);

    return true;
}

//TODO: Configurable CommandPool Create Flags if needed in future 
void vulkan_command_pool_allocate(RenderContext* context, VkCommandPool out_cmdPool, u32 queueFamilyIndex)
{
    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // - can reset cmdBuffs individually
    //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT - Hint that cmdBuffs are rerecorded with new cmdBuffs very often
    createInfo.queueFamilyIndex = queueFamilyIndex;
    
    VK_CHECK(vkCreateCommandPool(context->device_context.handle, &createInfo, context->allocator, &out_cmdPool));
}

void vulkan_command_buffer_begin(VkCommandBuffer cmdBuff, b8 onetimeSubmit)
{
    if (cmdBuff == VK_NULL_HANDLE)
        return;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = onetimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmdBuff, &beginInfo));
}

void vulkan_command_buffer_end(VkCommandBuffer cmdBuff)
{
    if (cmdBuff != VK_NULL_HANDLE)
        vkEndCommandBuffer(cmdBuff);
}

b8 vulkan_create_instance()
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

    const char* required_layer_names[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_KHR_dynamic_rendering"
    };

    u32 required_layer_count = sizeof(required_layer_names) / sizeof(const char*);

    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_properties(available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_properties.data()));

    for (u32 i = 0; i < required_layer_count; ++i) {
        std::cout << "Searching validation layer: " << required_layer_names[i] << "..." << std::endl;
        b8 is_found = false;;
        for (u32 l = 0; l < available_layer_count; ++l) {
            if (strcmp(required_layer_names[i], available_properties.at(l).layerName) == 0) {
                std::cout << "VULKAN INSTANCE LAYER: " << required_layer_names[i] << " found" << std::endl;
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            std::cout << "VULKAN INSTANCE LAYER: " << required_layer_names[i] << "is missing!\n";
            return false;
        }
    }

    inst_create_info.enabledLayerCount = required_layer_count;
    inst_create_info.ppEnabledLayerNames = required_layer_names;

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

void vulkan_create_debug_util_message()
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

b8 vulkan_create_surface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };

    createInfo.hinstance = platform_state->instance;
    createInfo.hwnd = platform_state->handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocator, &context.surface));
    return true;
#endif
    return false;
}

b8 load_exported_entry_points();
b8 load_global_level_function();
b8 load_instance_level_function();
b8 load_device_level_function();

b8 vulkan_renderer_init(internal_state* _platform_state)
{
#if defined(_WIN32)
    vulkan_library_loader = LoadLibrary("vulkan-1.dll");
#endif

    if (vulkan_library_loader == 0) {
        std::cout << "vulkan library failed to load" << std::endl;
        return false;
    }

    if (vulkan_library_loader == 0) {
        std::cout << "vulkan library failed to load" << std::endl;
        return false;
    }

    if (!load_exported_entry_points())
        return false;

    if (!load_global_level_function())
        return false;

    if (!vulkan_create_instance()) {
        std::cout << "create instance failed" << std::endl;
        return false;
    }

    if (!load_instance_level_function())
        return false;

    if (vulkan_create_surface()) {
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
    vulkan_create_debug_util_message();
#endif

    if (!vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain)) {
        std::cout << "create swapchain failed" << std::endl;
        return false;
    }

    platform_state = _platform_state;
    if (platform_state == nullptr) {
        std::cout << "platform state is nullptr" << std::endl;
        return false;
    }

    vulkan_renderer_init_imgui();

    for (u32 i = 0; i < MAX_FRAME; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(context.device_context.handle, &semaphore_create_info, context.allocator, &ready_to_render_semaphores[i]));
        VkFenceCreateInfo fence_create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(context.device_context.handle, &fence_create_info, context.allocator, &render_fences[i]);
        vulkan_command_pool_allocate(&context, cmdPools[i], context.device_context.graphics_family.index);
    }

    // initialize descriptor allocator
    // initialize command pool


    
    // initialize command buffer
}

void vulkan_renderer_load()
{
    // Load Shader
    vulkan_shader_module_create(&context, &basicVertShaderModule, "shader/test.vert");
    vulkan_shader_module_create(&context, &basicFragShaderModule, "shader/test.frag");

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
    shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[0].pNext = nullptr;
    shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[0].module = basicVertShaderModule;
    shaderStageCreateInfo[0].pName = "basicVertexShader";

    shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[1].pNext = nullptr;
    shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[1].module = basicFragShaderModule;
    shaderStageCreateInfo[1].pName = "basicFragmentShader";

    VkVertexInputBindingDescription vertexBindingDescriptions[2] = {};
    vertexBindingDescriptions[0].binding = 0;
    vertexBindingDescriptions[0].stride = sizeof(float) * 3;
    vertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexBindingDescriptions[1].binding = 1;
    vertexBindingDescriptions[1].stride = sizeof(float) * 2;
    vertexBindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttributeDescriptions[3] = {};
    // vertex position
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[0].offset = 0;
    
    // vertex normal
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = sizeof(float) * 3;

    // vertex uv
    vertexAttributeDescriptions[2].location = 0;
    vertexAttributeDescriptions[2].binding = 1;
    vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescriptions[2].offset = sizeof(float) * 6;

    //vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = sizeof(vertexBindingDescriptions) / sizeof(VkVertexInputBindingDescription);
    vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = sizeof(vertexAttributeDescriptions) / sizeof(VkVertexInputAttributeDescription);
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;
    
    //input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartzEnable = false;

    //viewport
    VkViewport viewport = {};
    float    width;
    float    height;
    float    minDepth;
    float    maxDepth;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = platform_state->width;
    viewport.height = platform_state->height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkOffset2D    offset;
    VkExtent2D    extent;
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = platform_state->width;
    scissor.extent.height = platform_state->height;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    //rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationStateCreateInfo.depthClampEnable = true;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = true;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = false;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateCreateInfo.lineWidth = 1.0f;

    //MSAA
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    //multisampling defaulted to no multisampling (1 sample per pixel)
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    //DepthStencil
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER; // REVERSE-Z PROJECTION
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.stencilTestEnable = false;
    //VkStencilOpState                          front;
    //VkStencilOpState                          back;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f; // OPTIONAL
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f; // OPTIONAL

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // OPTIONAL
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // OPTIONAL
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; //OPTIONAL
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendStateCreateInfo.logicOpEnable = false; //If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; 
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicStateCreateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    // textures
    VkDescriptorSetLayoutBinding bindings[1]; 
    bindings[0].binding = 0; // binding number
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[0].descriptorCount = context.device_context.properties.limits.maxDescriptorSetSampledImages;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    //const VkSampler* pImmutableSamplers;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    descriptorSetLayoutCreateInfo.pBindings = bindings;

    // Pipeline Layout (Descriptor Set needed)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &updateFreqNonedescriptorSetLayout;
    //TODO: pushConstant materialID
    //uint32_t                        pushConstantRangeCount;
    //const VkPushConstantRange* pPushConstantRanges;

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(context.device_context.handle, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    VkGraphicsPipelineCreateInfo basicPipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    basicPipelineCreateInfo.pNext = nullptr;
    basicPipelineCreateInfo.stageCount = sizeof(shaderStageCreateInfo) / sizeof(VkPipelineShaderStageCreateInfo);
    basicPipelineCreateInfo.pStages = shaderStageCreateInfo;
    basicPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    basicPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    basicPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    //const VkPipelineTessellationStateCreateInfo* pTessellationState;
    basicPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    basicPipelineCreateInfo.pMultisampleState;
    basicPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    basicPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    basicPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    basicPipelineCreateInfo.layout = pipelineLayout;
    basicPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    //basicPipelineCreateInfo.subpass = -1;
    //basicPipelineCreateInfo.basePipelineHandle;
    //basicPipelineCreateInfo.basePipelineIndex;
    
    VK_CHECK(vkCreateGraphicsPipelines(context.device_context.handle, VK_NULL_HANDLE, 1, &basicPipelineCreateInfo, nullptr, &basicPipeline));
}

void vulkan_renderer_update(f64 dt)
{
}

void vulkan_renderer_draw()
{
    currentFrame = frameCount % context.swapchain.max_frames_in_flight;

    VK_CHECK(vkWaitForFences(context.device_context.handle, 1, &render_fences[currentFrame], true, UINT64_MAX));
    VK_CHECK(vkResetFences(context.device_context.handle, 1, &render_fences[currentFrame]));

    u32 imageIndex = 0;

    if (!acquire_next_image_index_swapchain(
        &context,
        &context.swapchain,
        UINT64_MAX,
        image_available_semaphores[context.current_frame],
        0,
        &imageIndex))
    {
        std::cout << "acquire_next_image_index_swapchain failed" << std::endl;
        return;
    }

    vulkan_command_buffer_begin(cmdBuffs[currentFrame], true);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffs[currentFrame]);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool demo = true;
    ImGui::ShowDemoWindow(&demo);
    // draw gui

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();

    vulkan_command_buffer_end(cmdBuffs[currentFrame]);

    VkPipelineStageFlags wait_stages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[currentFrame];
    submit_info.pWaitDstStageMask = &wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdBuffs[currentFrame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &ready_to_render_semaphores[currentFrame];

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info, render_fences[currentFrame]));

    if (!present_image_swapchain(&context,
        &context.swapchain,
        context.device_context.present_queue,
        ready_to_render_semaphores[currentFrame],
        imageIndex)) {

        /*
        get_app_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);
        vulkan_swapchain_recreate(&context, context.framebuffer_width, context.framebuffer_height);
        regenerate_framebuffer();
        */
    }

}

b8 vulkan_renderer_draw_imgui()
{

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool demo = true;
    ImGui::ShowDemoWindow(&demo);

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();

    return true;
}

b8 vulkan_renderer_on_resize(u32 w, u32 h)
{
    if (context.device_context.handle != nullptr) {
        vkDeviceWaitIdle(context.device_context.handle);

		//get_app_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);
        context.framebuffer_width = w;
        context.framebuffer_height = h;

        if (!vulkan_swapchain_recreate(&context, context.framebuffer_width, context.framebuffer_height))
            return false;

        return true;
    }

	return false;
}

void vulkan_renderer_init_imgui()
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

    //this initializes imgui for SDL
    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*) { return vkGetInstanceProcAddr(context.instance, function_name); });
    ImGui_ImplWin32_Init(platform_state->handle);

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
    init_info.UseDynamicRendering = true;
    init_info.ColorAttachmentFormat = context.swapchain.image_format.format;

    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    //execute a gpu command to upload imgui font textures


    VkCommandBuffer cmdBuff = cmdBuffs[currentFrame];

    vulkan_command_buffer_begin(cmdBuff, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	ImGui_ImplVulkan_CreateFontsTexture();
    vulkan_command_buffer_end(cmdBuff);

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdBuff;

    VK_CHECK(vkQueueSubmit(context.device_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE)); 
	vkQueueWaitIdle(context.device_context.graphics_queue);
}

void vulkan_renderer_unload()
{
}

void vulkan_renderer_exit()
{
    vkQueueWaitIdle(context.device_context.graphics_queue);

    for (u32 i = 0; i < MAX_FRAME; ++i)
    {
        vkDestroyCommandPool(context.device_context.handle, cmdPools[i], context.allocator);
        cmdPools[i] = VK_NULL_HANDLE;
    }
       
    for (u32 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        vkDestroySemaphore(context.device_context.handle, image_available_semaphores[i], context.allocator);
        vkDestroySemaphore(context.device_context.handle, ready_to_render_semaphores[i], context.allocator);
        vkDestroyFence(context.device_context.handle, render_fences[i], context.allocator);

        image_available_semaphores[i] = VK_NULL_HANDLE;
        ready_to_render_semaphores[i] = VK_NULL_HANDLE;
        render_fences[i] = VK_NULL_HANDLE;
    }

    vulkan_swapchain_destroy(&context, &context.swapchain);

    // destroy imgui related objects
    vkDestroyDescriptorPool(context.device_context.handle, context.imgui_pool, context.allocator);
    ImGui_ImplVulkan_Shutdown();

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

