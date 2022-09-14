#include "default_scene.h"

#include "../vulkan_shader.h"
#include "../vulkan_mesh.h"

#include <iostream>

b8 default_scene::init(vulkan_context* api_context)
{
    vulkan_scene::init(api_context);

    main_shader->add_stage("test.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("test.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (main_shader->init() != true)
    {
        std::cout << "main_shader init fail" << std::endl;
        return false;
    }

    m_descriptor_builder = new descriptor_builder();
    m_descriptor_builder->begin(context->device_context.handle);

    graphics_pipeline = main_shader->pipeline;

    return true;
}

b8 default_scene::begin_frame(f32 dt)
{
    vulkan_scene::begin_frame(dt);

    return true;
}

b8 default_scene::end_frame()
{

    return vulkan_scene::end_frame();
}

b8 default_scene::begin_renderpass(u32 current_frame)
{
    vulkan_scene::begin_renderpass(current_frame);

    return true;
}

b8 default_scene::end_renderpass(u32 current_frame)
{
    return vulkan_scene::end_renderpass(current_frame);
}

b8 default_scene::draw()
{
    VkCommandBuffer command_buffer = graphics_commands.at(context->current_frame).buffer;

    // set viewport, scissor
    vulkan_scene::draw();

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.handle);

	model_constant model_constant{};

    for (const auto& obj : object_manager) {

        //TODO: this might go into draw call to use local transformation
		glm::mat4 model = obj.second->get_transform_matrix();
		glm::mat3 normal_matrix = glm::transpose(glm::inverse(model));

        model_constant.model = model;
        model_constant.normal_matrix = normal_matrix;
	    vkCmdPushConstants(command_buffer, main_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

        obj.second->draw(command_buffer);
    }

    /* TODO LIST
    * in the shader, only build pipeline layout / pipeline
    * 
    1. bind pipeline
    2. 
    */



    return b8();
}

void default_scene::shutdown()
{
    vulkan_scene::shutdown();

    m_descriptor_builder->cleanup();
}

b8 default_scene::on_resize(u32 w, u32 h)
{
    return vulkan_scene::on_resize(w, h);
}
