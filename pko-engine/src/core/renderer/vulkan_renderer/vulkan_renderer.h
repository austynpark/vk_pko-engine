#pragma once

#include "defines.h"
#include "core/renderer/renderer.h"

struct Command;


class VulkanRenderer : public Renderer {
	
public:
	VulkanRenderer() = delete;
	VulkanRenderer(void* platform_internal_state);
	~VulkanRenderer() override;

	b8 Init() override;
	void Load() override;
	void UnLoad() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void Shutdown() override;

private:
	void initImgui();
	b8 createInstance();
	void createDebugUtilMessage();
	b8 createSurface();

};

/*
* shader -
*/
