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
constexpr u32 MAX_SHADER_STAGE_COUNT = 3;
constexpr u32 MAX_COLOR_ATTACHMENT = 8;

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

    // Image
    // Dimension
    // Depth Bool
    // Arrayed Bool
    // MS
    // Sampled
    // Format
    // Access
};

struct ShaderReflection
{
    ShaderResource* pResources;
    u64 mResourceCount;
    VkShaderStageFlagBits mStageFlag;
    u8 mPushConstantIndex;
};

struct ShaderLoadDesc
{
    const char* mNames[MAX_SHADER_STAGE_COUNT];
};

struct Shader
{
    VkShaderStageFlags stageFlags;

    u32 mVertStageIndex;
    u32 mFragStageIndex;
    u32 mCompStageIndex;

    ShaderReflection* pShaderReflections[MAX_SHADER_STAGE_COUNT];
    ShaderModule* pShaderModules[MAX_SHADER_STAGE_COUNT];
    const char* mNames[MAX_SHADER_STAGE_COUNT];
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

    // VkPresentModeKHR present_mode;
    VkSurfaceFormatKHR surface_format;
    u32 image_count;
} VulkanSwapchain;

typedef struct VulkanRenderpass
{
    VkRenderPass handle;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} VulkanRenderpass;

typedef struct Pipeline
{
    VkPipeline handle;
    VkPipelineLayout layout;
} Pipeline;

class DescriptorAllocator
{
   public:
    DescriptorAllocator() = default;

    struct pool_sizes
    {
        std::vector<std::pair<VkDescriptorType, float>> sizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};

        VkDescriptorPool create_pool(VkDevice device, i32 count, VkDescriptorPoolCreateFlags flags);
    };

    void reset_pools();
    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

    void init(VkDevice new_device);

    void cleanup();

    VkDevice device = VK_NULL_HANDLE;

   private:
    VkDescriptorPool grab_pool();

    VkDescriptorPool current_pool{VK_NULL_HANDLE};
    pool_sizes descriptor_sizes;
    std::vector<VkDescriptorPool> used_pools;
    std::vector<VkDescriptorPool> free_pools;
};

class DescriptorLayoutCache
{
   public:
    DescriptorLayoutCache() = default;
    void init(VkDevice new_device);
    void cleanup();

    VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info);

    struct DescriptorLayoutInfo
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorLayoutInfo& other) const;

        size_t hash() const;
    };

   private:
    struct DescriptorLayoutHash
    {
        std::size_t operator()(const DescriptorLayoutInfo& info) const { return info.hash(); }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash>
        layout_cache;
    VkDevice device = VK_NULL_HANDLE;
};

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
    VulkanRenderpass main_renderpass;

    DescriptorAllocator* pDynamicDescriptorAllocators;
} VulkanContext;

#endif  // !VULKAN_TYPES_INL