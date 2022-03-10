#pragma once

#include "defines.h"
#include "core/renderer/renderer.h"

class vulkan_renderer : public renderer {
	
public:
	vulkan_renderer() = delete;
	vulkan_renderer(void* platform_internal_state);
	~vulkan_renderer() override;

	b8 init() override;
	b8 draw() override;
	void shutdown() override;
	b8 on_resize(u32 w, u32 h) override;
private:

	b8 create_instance();
	void create_debug_util_message();
	b8 create_surface();
};
