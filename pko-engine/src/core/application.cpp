#include "application.h"

#include "platform/platform.h"
#include "core/renderer/vulkan_renderer/vulkan_renderer.h"

#include "input.h"

#include <iostream>

static AppState app_state;
static ReloadDesc reload_desc = {ReloadType::RELOAD_TYPE_ALL};

b8 App::init(const char* app_name, i32 x, i32 y, u32 w, u32 h)
{
    app_state.platform_state = (PlatformState*)malloc(sizeof(PlatformState));

    app_state.width = w;
    app_state.height = h;
    app_state.delta_time = 0.0f;
    app_state.last_time = 0.0f;
    app_state.reload_type = ReloadType::RELOAD_TYPE_ALL;

    app_state.input_system = new InputSystem();

    if (!app_state.platform_state->init(app_name, x, y, w, h))
        return false;

    app_state.renderer = new VulkanRenderer(&app_state);

    if (!app_state.renderer->Init())
        return false;

    event_system::bind_event(event_code::EVENT_CODE_ONRESIZED, on_resize);

    return true;
}

b8 App::run()
{
    if (!app_state.platform_state->platform_message())
    {
        reload_desc = {app_state.reload_type};
        app_state.renderer->UnLoad(&reload_desc);
        app_state.renderer->Shutdown();

        return false;
    }

    if (app_state.reload_type != RELOAD_TYPE_UNDEFINED)
    {
        app_state.renderer->Load(&reload_desc);
        app_state.reload_type = RELOAD_TYPE_UNDEFINED;
    }

    f64 current_time = app_state.platform_state->get_absolute_time();
    app_state.delta_time = current_time - app_state.last_time;
    app_state.last_time = current_time;

    app_state.renderer->Update(app_state.delta_time);
    app_state.renderer->Draw();

    return true;
}

void App::shutdown()
{
    app_state.renderer->Shutdown();
    delete app_state.renderer;
    app_state.renderer = 0;

    delete app_state.input_system;
    app_state.input_system = 0;

    app_state.platform_state->shutdown();
    if (app_state.platform_state != NULL)
    {
        free(app_state.platform_state);
        app_state.platform_state = NULL;
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
