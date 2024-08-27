#pragma once

#include "defines.h"

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

struct AppState;
struct ReloadDesc;

class Renderer
{
public:
	Renderer() = delete;
	Renderer(AppState* state) : pAppState(state), frame_number(0) {};
	virtual ~Renderer() {};

	virtual b8 Init() = 0;
	virtual void Load(ReloadDesc* pDesc) = 0;
	virtual void UnLoad(ReloadDesc* pDesc) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	virtual	void Shutdown() = 0;
	//virtual b8 OnResize(u32 w, u32 h) = 0;



protected:
	// platform internal state
	AppState* pAppState;
	u32 frame_number;
};