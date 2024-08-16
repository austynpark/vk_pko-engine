#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"

#include "event.h"

class renderer;

typedef enum ReloadType
{
	RELOAD_TYPE_UNDEFINED = 0,
	RELOAD_TYPE_RESIZE = 0x1
};

typedef struct ReloadDesc
{
	ReloadType type;
};

struct application {
	
	static b8 init(const char* app_name ,i32 x, i32 y,u32 width, u32 height);
	static b8 run();
	static void shutdown();
	static b8 on_resize(u16 code, event_context pContext);
};

#endif // APPLICATION_H
