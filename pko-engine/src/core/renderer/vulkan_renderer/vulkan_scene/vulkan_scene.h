#ifndef VULKAN_SCENE_H
#define VULKAN_SCENE_H

#include "../vulkan_types.inl"

#include <memory>
#include <vector>

constexpr int scene_count = 1;

struct vulkan_context;
struct vulkan_swapchain;
class vulkan_shader;
class vulkan_render_object;
class skinned_mesh;

enum scene_name {
	E_DEFAULT_SCENE
};

/*
* TODO: might change this to pure virtual functions (does it really needs to be called by child?)
* every overriden function must call a parent function
*/
class vulkan_scene {

public:
	vulkan_scene();
	~vulkan_scene();
	
	//TODO: binding global data might need to be changed as virtual function
	// you must init 'graphics_pipeline' to whatever shader you want to bind global data
	virtual b8 init(vulkan_context* api_context);
	virtual b8 begin_frame(f32 dt);
	virtual b8 end_frame();
	
	// clear color / depth attachment and begin renderpass
	virtual b8 begin_renderpass(u32 current_frame);

	virtual b8 end_renderpass(u32 current_frame);

	// set scissor & viewport
	virtual b8 draw();
	virtual b8 draw_imgui();
	// destroy graphics_pipeline / framebuffer / main_renderpass / command_pool
	virtual void shutdown();

	virtual b8 on_resize(u32 w, u32 h);

	std::vector<VkFramebuffer>	framebuffers;
	u32 image_index;
	// per frame data
	std::vector<vulkan_command> graphics_commands;
	std::unique_ptr<vulkan_shader> main_shader;

	// pipeline where the global data is bound
	vulkan_pipeline graphics_pipeline;

protected:
	vulkan_context* context;

	std::unordered_map<const char*, std::unique_ptr<skinned_mesh>> object_manager;
	
	//TODO: might need texture manager

	void regenerate_framebuffer();
};

#endif // !VULKAN_SCENE_H
