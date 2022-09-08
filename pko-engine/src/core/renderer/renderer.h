#pragma once

#include "defines.h"

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

class renderer_frontend;
class vulkan_shader;

struct global_uniform_object {
	glm::mat4 projection;
	glm::mat4 view;
};

class renderer {
	friend renderer_frontend;
public:
	renderer() = delete;
	renderer(void* state) : plat_state(state), frame_number(0) {};
	virtual ~renderer() = 0 {};

	virtual b8 init() = 0;
	virtual b8 begin_frame(f32 dt) = 0;
	virtual b8 end_frame() = 0;
	virtual b8 begin_renderpass() = 0;
	virtual b8 end_renderpass() = 0;

	virtual	b8 draw() = 0;
	virtual	void shutdown() = 0;

	virtual b8 on_resize(u32 w, u32 h) = 0;

	virtual b8 add_shader(const char* name) = 0;

	// Must be called from child class
	virtual void update_global_data();

	virtual b8 bind_global_data() = 0;

	u32 width;
	u32 height;
protected:
	// platform internal state
	void* plat_state;
	u32 frame_number;
	
	global_uniform_object global_ubo;
};