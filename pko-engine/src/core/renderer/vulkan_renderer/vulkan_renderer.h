#pragma once

#include "defines.h"
#include "render_type.h"

#include <glm/glm.hpp>

struct internal_state;

b8 vulkan_renderer_init(internal_state* _platform_state);

// shader / Render Target, any sources that needs to be reloaded
void vulkan_renderer_load();
void vulkan_renderer_unload();
void vulkan_renderer_update(f64 dt);
void vulkan_renderer_draw();

void vulkan_renderer_exit();
