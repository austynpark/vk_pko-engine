#include "application.h"

#include "platform/platform.h"
#include "core/renderer/vulkan_renderer/vulkan_renderer.h"

#include "input.h"

#include <iostream>

struct application_state
{
	PlatformState platform;
	Renderer* renderer;
	input_system* input;

	f32 delta_time;
	f64 last_time;

	u32 width;
	u32 height;

	ReloadType reload_type;
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

	
	app_state.renderer = new VulkanRenderer(&app_state.platform);
	
	if (!app_state.renderer->Init())
		return false;

	event_system::bind_event(event_code::EVENT_CODE_ONRESIZED, on_resize);

	return true;
}

b8 application::run()
{
	if (!app_state.platform.platform_message()) {

		app_state.renderer->UnLoad();
		app_state.renderer->Shutdown();

		return false;
	}

	if (app_state.reload_type != RELOAD_TYPE_UNDEFINED)
	{
		ReloadDesc reload_desc = { app_state.reload_type };
		app_state.renderer->Load(&reload_desc);
		app_state.reload_type = RELOAD_TYPE_UNDEFINED;
	}

	f64 current_time = app_state.platform.get_absolute_time();
	app_state.delta_time = current_time - app_state.last_time;
	app_state.last_time = current_time;

	app_state.renderer->Update(app_state.delta_time);
	app_state.renderer->Draw();

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

	app_state.width = width;
	app_state.height = height;



	return true;
}
