#pragma once

#include "defines.h"

class renderer {
public:
	renderer() = delete;
	renderer(void* state) : plat_state(state) {};
	virtual ~renderer() = 0 {};

	virtual b8 init() = 0;
	virtual	b8 draw(f32 dt) = 0;
	virtual	void shutdown() = 0;

	virtual b8 on_resize() = 0;

protected:
	// platform internal state
	void* plat_state;
};