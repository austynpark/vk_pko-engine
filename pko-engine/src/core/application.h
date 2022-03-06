#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"
#include "platform/platform.h"

class renderer;

struct application {
	
	b8 init(const char* app_name ,i32 x, i32 y,u32 width, u32 height);
	b8 run();
	void shutdown();

	platform_state platform;
	renderer* renderer;

	f32 delta_time;
	f64 last_time;
};

#endif // APPLICATION_H
