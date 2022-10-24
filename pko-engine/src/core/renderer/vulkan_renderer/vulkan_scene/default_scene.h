#ifndef DEFAULT_SCENE_H
#define DEFAULT_SCENE_H

#include "vulkan_scene.h"
#include "core/mesh.h"

#include "../vulkan_descriptor_builder.h"

class default_scene : public vulkan_scene {
public:
	b8 init(vulkan_context* api_context) override;
	b8 begin_frame(f32 dt) override;
	b8 end_frame() override;
	b8 begin_renderpass(u32 current_frame) override;
	b8 end_renderpass(u32 current_frame) override;

	b8 draw() override;
	b8 draw_imgui() override;
	void shutdown() override;

	b8 on_resize(u32 w, u32 h) override;

	std::unique_ptr<vulkan_shader> debug_bone_shader;
	std::unique_ptr<vulkan_shader> line_shader;

	debug_draw_mesh path;
	std::unique_ptr<vulkan_render_object> line_debug_object;
private:
	VkDescriptorSet bone_transform_set;
	f32 delta_time = 0.0f;

	b8 single_model_draw_mode = true;
	const char* single_model_name;
	b8 enable_debug_draw = true;
};



#endif // !DEFAULT_SCENE_H
