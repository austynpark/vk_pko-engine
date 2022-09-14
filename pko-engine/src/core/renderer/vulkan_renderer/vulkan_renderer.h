#pragma once

#include "defines.h"
#include "core/renderer/renderer.h"

class vulkan_scene;

class vulkan_renderer : public renderer {
	
public:
	vulkan_renderer() = delete;
	vulkan_renderer(void* platform_internal_state);
	~vulkan_renderer() override;

	b8 init() override;

	b8 begin_frame(f32 dt) override;
	b8 end_frame() override;

	b8 begin_renderpass() override;
	b8 end_renderpass() override;

	b8 draw() override;
	b8 draw_imgui() override;
	void shutdown() override;
	b8 on_resize(u32 w, u32 h) override;

	b8 add_shader(const char* name) override;
	void update_global_data() override;
	b8 bind_global_data() override;

private:
	void init_imgui();

	b8 create_instance();
	void create_debug_util_message();
	b8 create_surface();

	std::vector<std::unique_ptr<vulkan_scene>> scene;
	u32 scene_index;

};

/*
* shader -
*/
