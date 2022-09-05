#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "defines.h"

#include <memory>
#include <vector>

class renderer;

class renderer_frontend {
public:
	renderer_frontend(void* platform_state);
	~renderer_frontend();

	b8 init();
	b8 draw(f32 dt);
	void shutdown();
private:
	renderer* renderer_backend;
	b8 load_meshes();
};

#endif // !RENDERER_FRONTEND_H
