#include "default_scene.h"

#include "../vulkan_shader.h"
#include "../vulkan_mesh.h"
#include "../vulkan_skinned_mesh.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <glm/gtc/type_ptr.hpp> 
#include <iostream>

b8 default_scene::init(vulkan_context* api_context)
{
    vulkan_scene::init(api_context);

    line_shader = std::make_unique<vulkan_shader>(api_context, &context->main_renderpass);

    line_shader->add_stage("debug.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("debug.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (line_shader->init(VK_PRIMITIVE_TOPOLOGY_LINE_LIST) != true) {
        std::cout << "line_shader init fail" << std::endl;
        return false;
    }

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
    delta_time = dt;

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

    //context->dynamic_descriptor_allocators[context->current_frame].

    for (const auto& obj : object_manager) {

        running_time += obj.second->animation_speed * delta_time;

        obj.second->update(running_time);

        VkDescriptorSet bone_transform_set;
        VkDescriptorBufferInfo buffer_info = obj.second->transform_buffer.get_info();

        descriptor_builder::begin(&context->layout_cache, &context->dynamic_descriptor_allocators[context->current_frame])
            .bind_buffer(0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT).build(bone_transform_set);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.layout, 1, 1, &bone_transform_set, 0, NULL);

		glm::mat4 model = obj.second->get_transform_matrix();
		glm::mat3 normal_matrix = glm::transpose(glm::inverse(model));
        
        model_constant.model = model;
        model_constant.normal_matrix = normal_matrix;
	    vkCmdPushConstants(command_buffer, main_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

        obj.second->draw(command_buffer);

        if (obj.second->enable_debug_draw) {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, line_shader->pipeline.handle);
            obj.second->draw_debug(command_buffer);
        }
    }

    return true;
}

b8 default_scene::draw_imgui()
{
    ImGui::Begin("CS460 Skeletal Animation");
    static const char* current_item = nullptr;
    if (ImGui::BeginTabBar("Tab Bar"))
    {
        if (ImGui::BeginTabItem("General"))
        {

            if (ImGui::BeginCombo("object", current_item))
            {
                for (const auto& obj : object_manager)
                {
                    bool is_selected =
                        (current_item ==
                            obj.first); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(obj.first, is_selected))
                        current_item = obj.first;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }

                // object combo
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Separator();
            if (current_item != nullptr) {

                auto& obj = object_manager[current_item];
                ImGui::SliderFloat3("translation", glm::value_ptr(obj->position), -100.0f, 100.0f);
                ImGui::SliderFloat3("rotation", glm::value_ptr(obj->rotation), -360.0f, 360.0f);
                ImGui::SliderFloat3("scale", glm::value_ptr(obj->scale), 0.0f, 1.0f);

                ImGui::Spacing();
                ImGui::Separator();

                if (obj->animation_count > 0) {
                    if (ImGui::CollapsingHeader("Animation"))
                    {
                        u32 min = 0;
                        u32 max = obj->animation_count - 1;
                        b8 is_changed = ImGui::SliderScalar(obj->animation->mName.C_Str(), ImGuiDataType_::ImGuiDataType_U32, &obj->selected_anim_index, &min, &max);
                        if (is_changed) {
                            obj->set_animation();
                        }

                        ImGui::SliderFloat("Anim speed", &obj->animation_speed, 0.0f, 2.0f);
                        ImGui::Checkbox("Bone Draw", &obj->enable_debug_draw);
                    }
                }
            }


            // General
            ImGui::EndTabItem();
        }
        //Tab Bar
        ImGui::EndTabBar();
    }


    ImGui::End();

    return true;
}

void default_scene::shutdown()
{
    line_shader->shutdown();

    vulkan_scene::shutdown();

}

b8 default_scene::on_resize(u32 w, u32 h)
{
    return vulkan_scene::on_resize(w, h);
}
