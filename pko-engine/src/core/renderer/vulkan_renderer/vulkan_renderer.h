#pragma once

#include "defines.h"
#include "core/renderer/renderer.h"

struct Command;

class VulkanRenderer : public Renderer {
	
public:
	VulkanRenderer() = delete;
	VulkanRenderer(AppState* pWndInternalState);
	~VulkanRenderer() override;

	b8 Init() override;
	void Load(ReloadDesc* pDesc) override;
	void UnLoad(ReloadDesc* pDesc) override;
	void Update(float deltaTime) override;
	void Draw() override;
	void Shutdown() override;
	//b8 OnResize(u32 w, u32 h) override;

private:
	void initImgui();
	b8 createInstance();
	void createDebugUtilMessage();
	b8 createSurface();

};

/*
* shader -
*/
