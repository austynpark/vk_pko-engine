#include "application.h"

#include "platform/platform.h"
#include "core/renderer/vulkan_renderer/vulkan_renderer.h"

#include "input.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

static AppState app_state;
static ReloadDesc reload_desc = { ReloadType::RELOAD_TYPE_ALL };

namespace
{
	void update_projection()
	{
		const f32 aspect =
			(app_state.height != 0) ? static_cast<f32>(app_state.width) / app_state.height : 1.0f;

		app_state.projection =
			glm::perspective(glm::radians(app_state.camera.zoom), aspect, 0.1f, 1000.0f);
		// Flip Y to account for Vulkan's clip space.
		app_state.projection[1][1] *= -1.0f;
	}

	void handle_camera_input(f32 delta_time)
	{
		if (!app_state.input_system)
		{
			return;
		}

		if (InputSystem::is_key_down(KEY_W))
		{
			app_state.camera.process_keyboard(camera_movement::FORWARD, delta_time);
		}
		if (InputSystem::is_key_down(KEY_S))
		{
			app_state.camera.process_keyboard(camera_movement::BACKWARD, delta_time);
		}
		if (InputSystem::is_key_down(KEY_A))
		{
			app_state.camera.process_keyboard(camera_movement::LEFT, delta_time);
		}
		if (InputSystem::is_key_down(KEY_D))
		{
			app_state.camera.process_keyboard(camera_movement::RIGHT, delta_time);
		}

		const f32 rotate_speed = 45.0f * delta_time;
		f32 yaw_offset = 0.0f;
		f32 pitch_offset = 0.0f;

		if (InputSystem::is_key_down(KEY_LEFT))
		{
			yaw_offset -= rotate_speed;
		}
		if (InputSystem::is_key_down(KEY_RIGHT))
		{
			yaw_offset += rotate_speed;
		}
		if (InputSystem::is_key_down(KEY_UP))
		{
			pitch_offset += rotate_speed;
		}
		if (InputSystem::is_key_down(KEY_DOWN))
		{
			pitch_offset -= rotate_speed;
		}

		if (yaw_offset != 0.0f || pitch_offset != 0.0f)
		{
			app_state.camera.process_keyboard_rotate(yaw_offset, pitch_offset);
		}
	}

	RenderFrameData build_frame_data()
	{
		RenderFrameData frame_data{};
		frame_data.delta_time = static_cast<f32>(app_state.delta_time);

		const glm::mat4 view = app_state.camera.get_view_matrix();
		frame_data.camera.view_proj = app_state.projection * view;

		return frame_data;
	}
}  // namespace

b8 App::init(const char* app_name, i32 x, i32 y, u32 w, u32 h)
{
	app_state.platform_state = (PlatformState*)malloc(sizeof(PlatformState));

	app_state.width = w;
	app_state.height = h;
	app_state.delta_time = 0.0f;
	app_state.last_time = 0.0f;
	app_state.reload_type = ReloadType::RELOAD_TYPE_ALL;

	app_state.input_system = new InputSystem();
	app_state.camera.init();
	update_projection();

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
		reload_desc = { app_state.reload_type };
		app_state.renderer->UnLoad(&reload_desc);

		return false;
	}

	if (app_state.reload_type != RELOAD_TYPE_UNDEFINED)
	{
		reload_desc.type = app_state.reload_type;
		app_state.renderer->Load(&reload_desc);
		app_state.reload_type = RELOAD_TYPE_UNDEFINED;
	}

	f64 current_time = app_state.platform_state->get_absolute_time();
	app_state.delta_time = current_time - app_state.last_time;
	app_state.last_time = current_time;

	update_projection();
	handle_camera_input(static_cast<f32>(app_state.delta_time));
	RenderFrameData frame_data = build_frame_data();
	app_state.renderer->Update(frame_data);
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
	app_state.reload_type = ReloadType::RELOAD_TYPE_RESIZE;
	update_projection();

	return true;
}
