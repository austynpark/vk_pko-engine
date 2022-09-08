#include "renderer.h"
#include "renderer_frontend.h"
#include "vulkan_renderer/vulkan_shader.h"
#include "vulkan_renderer/vulkan_renderer.h"

#include "vulkan_renderer/vulkan_mesh.h"
#include "spirv_helper.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

renderer_frontend::renderer_frontend(void* platform_state)
{
	renderer_backend = new vulkan_renderer(platform_state);
}

renderer_frontend::~renderer_frontend()
{
	delete renderer_backend;
}

b8 renderer_frontend::init(u32 w, u32 h)
{
	/*
	if (!load_meshes()) {
		std::cout << "load mesh failed" << std::endl;
		return false;
	}
	*/
	SpirvHelper::Init();

	renderer_backend->frame_number = 0;
	renderer_backend->width = w;
	renderer_backend->height = h;

	return renderer_backend->init();
}

b8 renderer_frontend::draw(f32 dt)
{
	renderer_backend->update_global_data();

	renderer_backend->begin_frame(dt);
	renderer_backend->begin_renderpass();

	renderer_backend->bind_global_data();
	renderer_backend->draw();

	renderer_backend->end_renderpass();
	b8 result = renderer_backend->end_frame();

	++renderer_backend->frame_number;

	return result;
}

b8 renderer_frontend::on_resize(u32 w, u32 h)
{
	renderer_backend->width = w;
	renderer_backend->height = h;

	renderer_backend->global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32)w / h, 0.1f, 100.0f);

	return 	renderer_backend->on_resize(w, h);
}

void renderer_frontend::shutdown()
{
	renderer_backend->shutdown();
	SpirvHelper::Finalize();
}

/*
b8 renderer_frontend::load_meshes()
{
	//TODO: if vulkan renderer
	renderer_backend->object_manager["test"] = std::make_unique<vulkan_render_object>("model/suzanne.obj");

	return true;
}
*/

void renderer::update_global_data()
{
	global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32)width / height, 0.1f, 100.0f);
    // camera pos, pos + front, up
    global_ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}
