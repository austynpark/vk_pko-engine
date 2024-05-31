#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "defines.h"

#include <memory>
#include <vector>

class Renderer;

class renderer_frontend {
public:
	renderer_frontend(void* platform_state);
	~renderer_frontend();

	b8 init(u32 w, u32 h);
	b8 load();
	b8 update(f32 dt);
	b8 draw();
	b8 on_resize(u32 w, u32 h);
	void shutdown();
	void unload();

private:
	Renderer* renderer_backend;
	
};

#endif // !RENDERER_FRONTEND_H
