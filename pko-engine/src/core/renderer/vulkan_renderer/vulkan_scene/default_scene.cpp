#include "default_scene.h"

#include "../vulkan_shader.h"

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
    return b8();
}

void default_scene::shutdown()
{
    vulkan_scene::shutdown();
    // delete graphics_pipeline
}

b8 default_scene::on_resize(u32 w, u32 h)
{
    return vulkan_scene::on_resize(w, h);
}
