#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

#include "renderer.h"
#include "renderer_frontend.h"
#include "camera.h"
#include "core/input.h"

#include "vulkan_renderer/vulkan_shader.h"
#include "vulkan_renderer/vulkan_renderer.h"
#include "vulkan_renderer/vulkan_mesh.h"

#include "spirv_helper.h"

static camera cam;

renderer_frontend::renderer_frontend(void* platform_state)
{
	renderer_backend = new vulkan_renderer(platform_state);
}

renderer_frontend::~renderer_frontend()
{
	delete renderer_backend;
}

b8 renderer_frontend::init(u32 w, u32 h)
{
	/*
	if (!load_meshes()) {
		std::cout << "load mesh failed" << std::endl;
		return false;
	}
	*/
	SpirvHelper::Init();

	cam.init();
	event_system::bind_event(event_code::EVENT_CODE_KEY_PRESSED, renderer_frontend::on_key_pressed);
	event_system::bind_event(event_code::EVENT_CODE_KEY_RELEASED, renderer_frontend::on_key_pressed);
	event_system::bind_event(event_code::EVENT_CODE_BUTTON_PRESSED, renderer_frontend::on_button_pressed);
	event_system::bind_event(event_code::EVENT_CODE_BUTTON_RELEASED, renderer_frontend::on_button_released);
	event_system::bind_event(event_code::EVENT_CODE_MOUSE_WHEEL, renderer_frontend::on_mouse_wheel);
	event_system::bind_event(event_code::EVENT_CODE_MOUSE_MOVED, renderer_frontend::on_mouse_moved);

	renderer_backend->frame_number = 0;
	renderer_backend->width = w;
	renderer_backend->height = h;

	return renderer_backend->init();
}

b8 renderer_frontend::draw(f32 dt)
{
	renderer_backend->update_global_data();

	renderer_backend->begin_frame(dt);
	renderer_backend->begin_renderpass();

	renderer_backend->bind_global_data();
	renderer_backend->draw();

	renderer_backend->end_renderpass();
	b8 result = renderer_backend->end_frame();

	++renderer_backend->frame_number;

	return result;
}

b8 renderer_frontend::on_resize(u32 w, u32 h)
{
	renderer_backend->width = w;
	renderer_backend->height = h;

	renderer_backend->global_ubo.projection = glm::perspective(glm::radians(cam.zoom), (f32)w / h, 0.1f, 100.0f);
	renderer_backend->global_ubo.projection[1][1] *= -1.0f;

	return 	renderer_backend->on_resize(w, h);
}

void renderer_frontend::on_keyboard_process(f32 dt)
{
	if (input_system::is_key_down(KEY_PLUS))
	{
		cam.process_zoom_inout(1.0f);
	}
	else if (input_system::is_key_down(KEY_MINUS))
	{
		cam.process_zoom_inout(-1.0f);
	}
	else if (input_system::is_key_down(KEY_DOWN) | input_system::is_key_down(KEY_NUMPAD2))
	{
		cam.process_keyboard_rotate(0.0f, -1.0f);
	}
	else if (input_system::is_key_down(KEY_UP) | input_system::is_key_down(KEY_NUMPAD8))
	{
		cam.process_keyboard_rotate(0.0f, 1.0f);
	}
	else if (input_system::is_key_down(KEY_LEFT) | input_system::is_key_down(KEY_NUMPAD4))
	{
		cam.process_keyboard_rotate(-1.0f, 0.0f);
	}
	else if (input_system::is_key_down(KEY_RIGHT) | input_system::is_key_down(KEY_NUMPAD6))
	{
		cam.process_keyboard_rotate(1.0f, 0.0f);
	}
}

void renderer_frontend::shutdown()
{
	renderer_backend->shutdown();
	SpirvHelper::Finalize();
}

/*
b8 renderer_frontend::load_meshes()
{
	//TODO: if vulkan renderer
	renderer_backend->object_manager["test"] = std::make_unique<vulkan_render_object>("model/suzanne.obj");

	return true;
}
*/

void renderer::update_global_data()
{

	global_ubo.projection = glm::perspective(glm::radians(cam.zoom), (f32)width / height, 0.1f, 100.0f);
	global_ubo.projection[1][1] *= -1.0f;
    // camera pos, pos + front, up
	global_ubo.view = cam.get_view_matrix();
	//glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}

b8 renderer_frontend::on_button_pressed(u16 code, event_context context)
{
	

	return true;
}

b8 renderer_frontend::on_button_released(u16 code, event_context context)
{


	return true;
}

b8 renderer_frontend::on_mouse_moved(u16 code, event_context context)
{
	

	return true;
}

b8 renderer_frontend::on_mouse_wheel(u16 code, event_context context)
{
	

	return true;
}

b8 renderer_frontend::on_key_pressed(u16 code, event_context context)
{
	u16 key = context.data.u16[0];


	switch (key) {
		/*
	case KEY_PLUS:
		//zoom in
		cam.process_zoom_inout(1.0f);
		break;
	case KEY_MINUS:
		//zoom out
		cam.process_zoom_inout(-1.0f);
		break;
	case KEY_NUMPAD2:
	case KEY_DOWN:
		// rotation around -Z
		cam.process_keyboard_rotate(-1.0f, 0.0f);
		break;
	case KEY_NUMPAD8:
	case KEY_UP:
		// rotation around +Z
		cam.process_keyboard_rotate(1.0f, 0.0f);
		break;
	case KEY_NUMPAD4:
	case KEY_LEFT:
		// rotation around -X
		cam.process_keyboard_rotate(0.0f, -1.0f);
		break;
	case KEY_NUMPAD6:
	case KEY_RIGHT:
		// rotation around X
		cam.process_keyboard_rotate(0.0f, 1.0f);
		break;
	case KEY_NUMPAD0:
		//cam.process_mouse_movement()
		break;
	case KEY_PERIOD:
		break;
		*/
	}


	return true;
}

b8 renderer_frontend::on_key_released(u16 code, event_context context)
{


	return b8();
}
