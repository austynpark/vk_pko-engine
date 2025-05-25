#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"

#include "event.h"


struct PlatformState;
class InputSystem;
class Renderer;

typedef enum ReloadType
{
	RELOAD_TYPE_UNDEFINED = 0,
	RELOAD_TYPE_RESIZE = 0x1,
	RELOAD_TYPE_ALL = UINT32_MAX
} ReloadType;

typedef struct ReloadDesc
{
	ReloadType type;
} ReloadDesc;

struct AppState
{
	PlatformState* platform_state;
	Renderer* renderer;
	InputSystem* input_system;

	f32 delta_time;
	f64 last_time;

	u32 width;
	u32 height;

	ReloadType reload_type;
};

struct App {
	
	static b8 init(const char* app_name ,i32 x, i32 y,u32 width, u32 height);
	static b8 run();
	static void shutdown();
	static b8 on_resize(u16 code, event_context context);
};

#endif // APPLICATION_H
