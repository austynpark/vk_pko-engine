#pragma once

#include "core/renderer/renderer.h"
#include "defines.h"

struct Command;

class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer() = delete;
	VulkanRenderer(AppState* pWndInternalState);
	~VulkanRenderer() override;

	b8 Init() override;
	void Load(ReloadDesc* desc) override;
	void UnLoad(ReloadDesc* desc) override;
	void Update(const RenderFrameData& frame_data) override;
	void Draw() override;
	void Shutdown() override;
	// b8 OnResize(u32 w, u32 h) override;

private:
	b8 beginFrame();
	void basePass();
	void lightingPass();
	void debugPass();
	void endFrame();

	void createShader();
	void createPipeline();
	void createRenderTarget();
	void createBuffer();
	void createSceneDescriptors();
	void createGBufferDescriptors();

	void destroyShader();
	void destroyPipeline();
	void destroyRenderTarget();
	void destroyBuffer();
	void destroySceneDescriptors();
	void destroyGBufferDescriptors();
	void destroyMeshResources();

	void initImgui();
	b8 createInstance();
	void createDebugUtilMessage();
	b8 createSurface();
};

/*
 * shader -
 */
