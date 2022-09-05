#ifndef DEFAULT_SCENE_H
#define DEFAULT_SCENE_H

#include "vulkan_scene.h"

class default_scene : public vulkan_scene {
public:
	b8 init(vulkan_context* api_context) override;
	b8 begin_frame(f32 dt) override;
	b8 end_frame() override;
	b8 begin_renderpass(u32 current_frame) override;
	b8 end_renderpass(u32 current_frame) override;

	b8 draw() override;
	void shutdown() override;

	b8 on_resize(u32 w, u32 h) override;
private:

};



#endif // !DEFAULT_SCENE_H
