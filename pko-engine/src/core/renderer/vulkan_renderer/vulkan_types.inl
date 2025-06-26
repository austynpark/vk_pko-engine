#ifndef VULKAN_TYPES_INL
#define VULKAN_TYPES_INL

#include "defines.h"
#include "vulkan_functions.h"

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <cassert>
#include <unordered_map>
#include <vector>

#define VK_CHECK(call)                 \
    do                                 \
    {                                  \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

constexpr u32 MAX_FRAME = 3;
constexpr u32 MAX_COLOR_ATTACHMENT = 8;
constexpr u32 MAX_DESCRIPTOR_SET_LAYOUT = 4;

typedef union ClearValue
{
    struct
    {
        float r;
        float g;
        float b;
        float a;
    };
    struct
    {
        float depth;
        u32 stencil;
    };
} ClearValue;

typedef enum QueueType
{
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_PRESENT,
    QUEUE_TYPE_TRANSFER,
    QUEUE_TYPE_COMPUTE
} QueueType;

typedef struct QueueFamily
{
    u32 index = u32(-1);
    u32 count;
} QueueFamily;

typedef struct DeviceContext
{
    VkDevice handle;
    VkPhysicalDevice physical_device;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;

    QueueFamily graphics_family;
    QueueFamily present_family;
    QueueFamily transfer_family;
    QueueFamily compute_family;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
    VkQueue compute_queue;

    VkFormat depth_format;
} DeviceContext;

struct Image
{
    VkImage handle;
    VkImageView view;
    VmaAllocation allocation;
    u32 width;
    u32 height;
};

typedef struct Command
{
    VkCommandPool pool;
    VkCommandBuffer buffer;
    QueueType type;
    bool is_rendering;
} Command;

// This part of code is from https://github.com/ConfettiFX/The-Forge.
typedef enum ResourceState
{
    RESOURCE_STATE_UNDEFINED = 0x0,
    RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
    RESOURCE_STATE_INDEX_BUFFER = 0x2,
    RESOURCE_STATE_RENDER_TARGET = 0x4,
    RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
    RESOURCE_STATE_DEPTH_WRITE = 0x10,
    RESOURCE_STATE_DEPTH_READ = 0x20,
    RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
    RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
    RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
    RESOURCE_STATE_PRESENT = 0x200,
    RESOURCE_STATE_COPY_DEST = 0x400,
    RESOURCE_STATE_COPY_SOURCE = 0x800,
    RESOURCE_STATE_COMMON = 0x1000
} ResourceState;

typedef struct Buffer
{
    VkBuffer handle;
    VmaAllocation allocation;
} Buffer;

typedef __declspec(align(32)) struct TextureDesc
{
    u32 width : 16;
    u32 height : 16;
    u32 mip_levels;
    u32 sample_count;
    VkFormat vulkan_format;
    ClearValue clear_value;
    ResourceState start_state;
    VkDescriptorType type;

    const void* native_handle;
} TextureDesc;

typedef __declspec(align(32)) struct Texture
{
    VkImage image;
    VkImageView srv_descriptor;
    VkImageView* uav_descriptors;

    VmaAllocation pAlloc;

    u32 width : 16;
    u32 height : 16;
    u32 vulkan_format : 8;
    u32 sample_count : 4;
    u32 aspect_mask : 4;
    u32 mip_levels : 5;
    u32 owns_image : 1;
} Texture;

typedef __declspec(align(32)) struct RenderTargetDesc
{
    u32 width;
    u32 height;
    u32 mip_levels;
    u32 sample_count;
    VkFormat vulkan_format;
    ClearValue clear_value;
    ResourceState start_state;
    // For Descriptor
    VkDescriptorType descriptor_type;
    const void* native_handle;  // VkImage

} RenderTargetDesc;

typedef __declspec(align(64)) struct RenderTarget
{
    Texture* texture;
    ClearValue clear_value;
    u32 array_size : 16;
    u32 depth : 16;
    u32 width : 16;
    u32 height : 16;
    u32 descriptors : 20;
    u32 mip_levels : 10;
    u32 sample_quality : 5;
    VkFormat vulkan_format;
    VkSampleCountFlagBits sample_count;
    VkImageView descriptor;
    VkImageView* array_descriptors;

} RenderTarget;

typedef struct RenderTargetBarrier
{
    RenderTarget* render_target;
    ResourceState current_state;
    ResourceState new_state;
} RenderTargetBarrier;

typedef struct TextureBarrier
{
    Texture* texture;
    ResourceState current_state;
    ResourceState new_state;
} TextureBarrier;

typedef struct BufferBarrier
{
    Buffer* buffer;
    ResourceState current_state;
    ResourceState new_state;
} BufferBarrier;

typedef struct RenderTargetOperator
{
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
} RenderTargetOperator;

typedef struct RenderDesc
{
    // Bind RenderTarget
    RenderTarget** render_targets;
    VkClearValue clear_color;
    u32 render_target_count;
    VkRect2D render_area;

    RenderTarget* depth_target;
    VkClearValue clear_depth;
    bool is_depth_stencil;

    RenderTarget* stencil_target;
    VkClearValue clear_stencil;

    // Order sensitive
    RenderTargetOperator* render_target_operators;
};

typedef enum ShaderStage
{
    SHADER_STAGE_VERTEX,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_COMPUTE,
    MAX_SHADER_STAGE_COUNT
} ShaderStage;

struct ShaderModule
{
    VkShaderModule module;
    std::vector<u32> codes;
    u32 code_size;
};

struct ShaderVariable
{
    enum Type
    {
        SHADER_VARIABLE_FLOAT_TYPE,
        SHADER_VARIABLE_INTEGER_TYPE,
        SHADER_VARIABLE_UNSIGNED_TYPE,
        SHADER_VARIABLE_MAT3X3_TYPE,
        SHADER_VARIABLE_MAT4X4_TYPE,
        // SHADER_VARIABLE_
    };

    b8 is_depth;
    b8 is_array;
    b8 mMSAA;
    b8 is_sampled;
    Type type;
    const char* name;

    u32 mOffset;
    u32 mSize;
};

struct ShaderResource
{
    u32 mSet;
    u32 mBinding;
    const char* name;
    b8 mIsStruct;

    VkDescriptorType type;
    u32 mIndex;

    u32 mSize;
    u32 mMemberCount;
    ShaderVariable* mMembers;
    VkShaderStageFlags stage_flags;
};

struct ShaderReflection
{
    ShaderResource* resources;
    u64 resource_count;
    VkShaderStageFlagBits stage_flag;
    // u8 mPushConstantIndex;
};

struct ShaderLoadDesc
{
    const char* names[MAX_SHADER_STAGE_COUNT];
};

struct Shader
{
    VkShaderStageFlags stage_flags;

    u32 vert_stage_index;
    u32 frag_stage_index;
    u32 comp_stage_index;

    ShaderResource* shader_resources;
    u64 resource_count;
    ShaderModule* shader_modules[MAX_SHADER_STAGE_COUNT];
    const char* names[MAX_SHADER_STAGE_COUNT];
};

typedef struct VulkanSwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR surface_capabilites;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> present_modes;

    u32 format_count;
    u32 present_mode_count;
} VulkanSwapchainSupportInfo;

typedef __declspec(align(16)) struct SwapchainDesc
{
    void* phWnd;
    u32 width : 16;
    u32 height : 16;
    u32 image_count : 2;  // MAX_FRAMES (TRIPPLE BUFFERING)
} SwapchainDesc;

typedef struct VulkanSwapchain
{
    RenderTarget** render_targets;

    VkSwapchainKHR handle;
    SwapchainDesc* desc;

    VkSurfaceFormatKHR surface_format;
    u32 image_count;
} VulkanSwapchain;

typedef struct VertexInputBinding
{
    VkVertexInputRate input_rate;
    u32 binding;
    u32 stride;
} VertexInputBinding;

typedef struct VertexInputAttribute
{
    u32 location;
    u32 binding;
    VkFormat format;
    u32 offset;
} VertexInputAttribute;

typedef struct PipelineInputDesc
{
    VkPrimitiveTopology topology;
    bool is_primitive_restart;

    VertexInputBinding* bindings;
    u32 binding_count;
    VertexInputAttribute* attributes;
    u32 attribute_count;
} PipelineInputDesc;

typedef struct RasterizeDesc
{
    u32 depth_clamp_enable;
    u32 rasterizer_discard_enable;

    VkPolygonMode polygon_mode;
    VkCullModeFlagBits cull_mode;
    VkFrontFace front_face;

    u32 depth_bias_enable;
    f32 depth_bias_constant_factor;
    f32 depth_bias_clamp;
    f32 depth_bias_slope_factor;

    f32 line_width;
} RasterizeDesc;

typedef struct DepthStencilDesc
{
    u32 depth_test_enable;
    u32 depth_write_enable;
    u32 depth_bounds_test_enable;
    u32 stencil_test_enable;

    VkCompareOp depth_compare_op;
    VkStencilOpState stencil_front_op_state;
    VkStencilOpState stencil_back_op_state;

    f32 min_depth_bounds;
    f32 max_depth_bounds;
} DepthStencilDesc;

typedef enum ColorBlendMode
{
    COLOR_BLEND_OPAQUE,
    COLOR_BLEND_ALPHA,
    COLOR_BLEND_ADDITIVE,
    COLOR_BLEND_MULTIPLICATIVE,
} ColorBlendMode;

typedef struct DescritporSetDesc
{
    u32 binding_count;

} DescriptorSetDesc;

typedef struct DescriptorHashMap
{
    const char* key;
    u32 value;
} DescriptorHashMap;

typedef struct PipelineLayout
{
    DescriptorHashMap* descriptor_map;
    VkDescriptorSetLayout set_layouts[MAX_DESCRIPTOR_SET_LAYOUT];
    VkPipelineLayout handle;
} DescriptorSetLayout;

typedef struct PipelineDesc
{
    RasterizeDesc* rasterize_desc;
    DepthStencilDesc* depth_stencil_desc;
    PipelineInputDesc* input_desc;
    PipelineLayout* layout;
    Shader* shader;

    ColorBlendMode* blend_modes;
    u32 color_attachment_count;
    VkFormat* color_attachment_formats;
    VkFormat depth_attachment_format;
    VkFormat stencil_attachment_format;
    // view mask
} PipelineDesc;

typedef struct Pipeline
{
    VkPipeline handle;
} Pipeline;

typedef struct RenderContext
{
    VmaAllocator vma_allocator;
    VkAllocationCallbacks* allocator;
    VkInstance instance;

    // imgui
    VkDescriptorPool imgui_pool;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif  //_DEBUG

    u32 current_frame;
    u32 image_index;

    u32 framebuffer_width;
    u32 framebuffer_height;

    VkSurfaceKHR surface;
    DeviceContext device_context;
    VulkanSwapchainSupportInfo swapchain_support_info;

} VulkanContext;

#endif  // !VULKAN_TYPES_INL