#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"

#include "event.h"

class renderer;

struct application {
	
	static b8 init(const char* app_name ,i32 x, i32 y,u32 width, u32 height);
	static b8 run();
	static void shutdown();
	static b8 on_resize(u16 code, event_context context);
};

void get_app_framebuffer_size(u32* w, u32* h);

#endif // APPLICATION_H
