#include "application.h"

#include "platform/platform.h"
#include "renderer/renderer_frontend.h"
#include "object_manager.h"

#include "input.h"

#include <iostream>

struct application_state
{
	platform_state platform;
	renderer_frontend* renderer;
	input_system* input;

	f32 delta_time;
	f64 last_time;

	u32 width;
	u32 height;
};

static application_state app_state;

b8 application::init(const char* app_name ,i32 x, i32 y,u32 w, u32 h)
{
	app_state.width = w;
	app_state.height = h;
	app_state.delta_time = 0.0f;
	app_state.last_time = 0.0f;

	app_state.input = new input_system();

	if (!app_state.platform.init(app_name, x, y, w, h))
		return false;

	
	app_state.renderer = new renderer_frontend(app_state.platform.state);
	
	if (!app_state.renderer->init())
		return false;

	event_system::bind_event(event_code::EVENT_CODE_ONRESIZED, on_resize);

	return true;
}

b8 application::run()
{
	if (!app_state.platform.platform_message()) {
		return false;
	}

	f64 current_time = app_state.platform.get_absolute_time();
	app_state.delta_time = current_time - app_state.last_time;
	app_state.last_time = current_time;

	app_state.renderer->draw(app_state.delta_time);

	return true;
}

void application::shutdown()
{
	delete app_state.renderer;
	app_state.renderer = 0;
	app_state.platform.shutdown();
	delete app_state.input;
}

b8 application::on_resize(u16 code, event_context context)
{
	u16 width = context.data.u32[0];
	u16 height = context.data.u32[1];

	b8 result =	app_state.renderer->on_resize(width, height);

	if (!result) return false;

	app_state.width = width;
	app_state.height = height;

	return true;
}

void get_app_framebuffer_size(u32* w, u32* h)
{
	*w = app_state.width;
	*h = app_state.height;
}
