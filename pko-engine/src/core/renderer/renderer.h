#pragma once

#include "defines.h"

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

class PlatformState;
struct ReloadDesc;

class Renderer
{
public:
	Renderer() = delete;
	Renderer(PlatformState* state) : platform_state(state), frame_number(0), width(0), height(0) {};
	virtual ~Renderer() {};

	virtual b8 Init() = 0;
	virtual void Load(ReloadDesc* desc) = 0;
	virtual void UnLoad() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	virtual	void Shutdown() = 0;
	virtual b8 OnResize(u32 w, u32 h) = 0;



protected:
	// platform internal state
	PlatformState* platform_state;
	u32 frame_number;
	u32 width;
	u32 height;
};