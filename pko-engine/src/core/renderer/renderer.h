#pragma once

#include "defines.h"

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

class renderer_frontend;
class object;
//#include "vulkan_renderer/vulkan_mesh.h"

struct global_uniform_object {
	glm::mat4 projection;
	glm::mat4 view;
};

class renderer {
	friend renderer_frontend;
public:
	renderer() = delete;
	renderer(void* state) : plat_state(state) {};
	virtual ~renderer() = 0 {};

	virtual b8 init() = 0;
	virtual	b8 draw() = 0;
	virtual	void shutdown() = 0;

	virtual b8 on_resize(u32 w, u32 h) = 0;

protected:
	// platform internal state
	void* plat_state;
	u32 frame_number;
	
	std::unordered_map<const char*, std::unique_ptr<object>> object_manager;

	global_uniform_object global_ubo;

};