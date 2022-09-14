#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "defines.h"

#include "core/event.h"

#include <memory>
#include <vector>

class renderer;

class renderer_frontend {
public:
	renderer_frontend(void* platform_state);
	~renderer_frontend();

	b8 init(u32 w, u32 h);
	b8 draw(f32 dt);
	b8 on_resize(u32 w, u32 h);
	void on_keyboard_process(f32 dt);
	void shutdown();

	static b8 on_button_pressed(u16 code, event_context context);
	static b8 on_button_released(u16 code, event_context context);
	static b8 on_mouse_moved(u16 code, event_context context);
	static b8 on_mouse_wheel(u16 code, event_context context);
	static b8 on_key_pressed(u16 code, event_context context);
	static b8 on_key_released(u16 code, event_context context);

private:
	renderer* renderer_backend;
	
};

#endif // !RENDERER_FRONTEND_H
