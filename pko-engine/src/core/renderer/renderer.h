#pragma once

#include "defines.h"

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

class PlatformState;

class Renderer
{
public:
	Renderer() = delete;
	Renderer(void* state) : platform_state(state), frame_number(0) {};
	virtual ~Renderer() {};

	virtual b8 Init() = 0;
	virtual void Load(/*Load Type Desc*/) = 0;
	virtual void UnLoad() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	virtual	void Shutdown() = 0;
	virtual b8 OnResize(u32 w, u32 h) = 0;

	u32 width;
	u32 height;

protected:
	// platform internal state
	PlatformState* platform_state;
	u32 frame_number;
};