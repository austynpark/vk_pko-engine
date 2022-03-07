#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"

class renderer;

struct application {
	
	b8 init(const char* app_name ,i32 x, i32 y,u32 width, u32 height);
	b8 run();
	void shutdown();
	//b8 on_resize();
};

void get_app_framebuffer_size(u32* w, u32* h);

#endif // APPLICATION_H
