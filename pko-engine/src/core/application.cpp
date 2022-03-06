#include "application.h"
#include "platform/platform.h"

#include "renderer/renderer.h"
#include "renderer/vulkan_renderer/vulkan_renderer.h"

#include <iostream>

b8 application::init(const char* app_name ,i32 x, i32 y,u32 width, u32 height)
{
	delta_time = 0.0f;
	last_time = 0.0f;

	if (!platform.init(app_name, x, y, width, height))
		return false;

	renderer = new vulkan_renderer(platform.state);

	if (!renderer->init())
		return false;

	return true;
}

b8 application::run()
{
	if (!platform.platform_message()) {
		return false;
	}

	f64 current_time = platform.get_absolute_time();
	delta_time = current_time - last_time;
	last_time = current_time;

	renderer->draw(delta_time);

	return true;
}

void application::shutdown()
{
	delete renderer;
	renderer = 0;
	platform.shutdown();
}
