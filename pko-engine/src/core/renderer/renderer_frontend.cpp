#include "renderer_frontend.h"

#include "renderer.h"
#include "vulkan_renderer/vulkan_renderer.h"

#include "core/object.h"
#include "vulkan_renderer/vulkan_mesh.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

renderer_frontend::renderer_frontend(void* platform_state)
{
	renderer_backend = new vulkan_renderer(platform_state);
}

renderer_frontend::~renderer_frontend()
{
	delete renderer_backend;
}

b8 renderer_frontend::init()
{
	if (!load_meshes()) {
		std::cout << "load mesh failed" << std::endl;
		return false;
	}

	renderer_backend->global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32) / context.framebuffer_height, 0.1f, 100.0f);
    // camera pos, pos + front, up
	renderer_backend->global_ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));


	return renderer_backend->init();
}

b8 renderer_frontend::draw(f32 dt)
{
	return renderer_backend->draw();
}

b8 renderer_frontend::on_resize(u32 w, u32 h)
{
	renderer_backend->global_ubo.projection = glm::perspective(glm::radians(45.0f), (f32)w / h, 0.1f, 100.0f);

	return 	renderer_backend->on_resize(w, h);
}

void renderer_frontend::shutdown()
{
	renderer_backend->shutdown();
}

b8 renderer_frontend::load_meshes()
{
	//TODO: if vulkan renderer
	renderer_backend->object_manager["test"] = std::make_unique<vulkan_render_object>("model/suzanne.obj");

	return true;
}
