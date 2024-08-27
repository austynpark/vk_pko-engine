#include "application.h"

#include "platform/platform.h"
#include "core/renderer/vulkan_renderer/vulkan_renderer.h"

#include "input.h"

#include <iostream>

static AppState app_state;
static ReloadDesc reload_desc = { ReloadType::RELOAD_TYPE_ALL };

b8 App::init(const char* app_name ,i32 x, i32 y,u32 w, u32 h)
{
	app_state.pPlatform = (PlatformState*)malloc(sizeof(PlatformState));

	app_state.width = w;
	app_state.height = h;
	app_state.delta_time = 0.0f;
	app_state.last_time = 0.0f;

	app_state.pInputSystem = new InputSystem();

	if (!app_state.pPlatform->init(app_name, x, y, w, h))
		return false;

	
	app_state.pRenderer = new VulkanRenderer(&app_state);
	
	if (!app_state.pRenderer->Init())
		return false;

	event_system::bind_event(event_code::EVENT_CODE_ONRESIZED, on_resize);

	return true;
}

b8 App::run()
{
	if (!app_state.pPlatform->platform_message()) {

		reload_desc = { ReloadType::RELOAD_TYPE_ALL };
		app_state.pRenderer->UnLoad(&reload_desc);
		app_state.pRenderer->Shutdown();

		return false;
	}

	if (app_state.reload_type != RELOAD_TYPE_UNDEFINED)
	{
		app_state.pRenderer->Load(&reload_desc);
		app_state.reload_type = RELOAD_TYPE_UNDEFINED;
	}

	f64 current_time = app_state.pPlatform->get_absolute_time();
	app_state.delta_time = current_time - app_state.last_time;
	app_state.last_time = current_time;

	app_state.pRenderer->Update(app_state.delta_time);
	app_state.pRenderer->Draw();

	return true;
}

void App::shutdown()
{
	app_state.pRenderer->Shutdown();
	delete app_state.pRenderer;
	app_state.pRenderer = 0;

	delete app_state.pInputSystem;
	app_state.pInputSystem = 0;

	app_state.pPlatform->shutdown();
	if (app_state.pPlatform != NULL)
	{
		free(app_state.pPlatform);
		app_state.pPlatform = NULL;
	}
	
}

b8 App::on_resize(u16 code, event_context context)
{
	u16 width = context.data.u32[0];
	u16 height = context.data.u32[1];

	app_state.width = width;
	app_state.height = height;

	return true;
}
