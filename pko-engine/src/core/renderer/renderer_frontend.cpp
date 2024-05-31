#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

#include "renderer.h"
#include "renderer_frontend.h"
#include "camera.h"
#include "core/input.h"
#include "core/event.h"

#include "vulkan_renderer/vulkan_shader.h"
#include "vulkan_renderer/vulkan_renderer.h"
#include "vulkan_renderer/vulkan_mesh.h"

#include "spirv_helper.h"

static camera cam;

renderer_frontend::renderer_frontend(void* platform_state)
{
	renderer_backend = new VulkanRenderer(platform_state);
}

renderer_frontend::~renderer_frontend()
{
	delete renderer_backend;
}

b8 renderer_frontend::init(u32 w, u32 h)
{
	SpirvHelper::Init();

	cam.init();
	//event_system::bind_event(event_code::EVENT_CODE_KEY_PRESSED, renderer_frontend::on_key_pressed);
	//event_system::bind_event(event_code::EVENT_CODE_KEY_RELEASED, renderer_frontend::on_key_pressed);
	//event_system::bind_event(event_code::EVENT_CODE_BUTTON_PRESSED, renderer_frontend::on_button_pressed);
	//event_system::bind_event(event_code::EVENT_CODE_BUTTON_RELEASED, renderer_frontend::on_button_released);
	//event_system::bind_event(event_code::EVENT_CODE_MOUSE_WHEEL, renderer_frontend::on_mouse_wheel);
	//event_system::bind_event(event_code::EVENT_CODE_MOUSE_MOVED, renderer_frontend::on_mouse_moved);

	renderer_backend->frame_number = 0;
	renderer_backend->width = w;
	renderer_backend->height = h;

	return renderer_backend->init();
}

b8 renderer_frontend::load()
{
	


	return true;
}

b8 renderer_frontend::update(f32 dt)
{
	return b8();
}

b8 renderer_frontend::draw()
{
	renderer_backend->draw_imgui();
	renderer_backend->bind_global_data();

	renderer_backend->draw();

	++renderer_backend->frame_number;

	return true;
}

b8 renderer_frontend::on_resize(u32 w, u32 h)
{
	renderer_backend->width = w;
	renderer_backend->height = h;

	renderer_backend->global_ubo.projection = glm::perspective(glm::radians(cam.zoom), (f32)w / h, 0.1f, 100.0f);
	renderer_backend->global_ubo.projection[1][1] *= -1.0f;

	return 	renderer_backend->on_resize(w, h);
}

void renderer_frontend::shutdown()
{
	renderer_backend->shutdown();
	SpirvHelper::Finalize();
}

void renderer_frontend::unload()
{
}
