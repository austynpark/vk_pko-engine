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

    debug_bone_shader = std::make_unique<vulkan_shader>(api_context, &context->main_renderpass);
    line_shader = std::make_unique<vulkan_shader>(api_context, &context->main_renderpass);

    debug_bone_shader->add_stage("debug.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("debug.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (debug_bone_shader->init(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false) != true) {
        std::cout << "debug_bone_shader init fail" << std::endl;
        return false;
    }

    line_shader->add_stage("debug_line.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("debug_line.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

    
    vertex_input_description line_input;
    line_input.bindings.push_back(
        { 
            0,
            sizeof(glm::vec3),
            VK_VERTEX_INPUT_RATE_VERTEX 
        }
    );

    line_input.attributes.push_back(
        {
            0,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            0 
        }
    );

    if (line_shader->init(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false, &line_input) != true) {
        std::cout << "line_shader init fail" << std::endl;
        return false;
    }

    debug_mesh_add_line(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), &path);
    debug_mesh_add_line(glm::vec3(-1.0f, -1.0f, 0.0f), &path);
    debug_mesh_add_line(glm::vec3(1.0f, -1.0f, 0.0f), &path);
    debug_mesh_link_begin_end(&path);

    line_debug_object = std::make_unique<vulkan_render_object>(context, &path);

    main_shader->add_stage("test.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("test.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (main_shader->init() != true)
    {
        std::cout << "main_shader init fail" << std::endl;
        return false;
    }

    graphics_pipeline = main_shader->pipeline;
    single_model_name = "";

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

	model_constant model_constant{};

    if (!single_model_draw_mode) {
        for (const auto& obj : object_manager) {
            obj.second->update(delta_time);

            VkDescriptorSet bone_transform_set;
            VkDescriptorBufferInfo buffer_info = obj.second->transform_buffer.get_info();

            descriptor_builder::begin(&context->layout_cache, &context->dynamic_descriptor_allocators[context->current_frame])
                .bind_buffer(0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT).build(bone_transform_set);

			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.handle);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.layout, 1, 1, &bone_transform_set, 0, NULL);

            glm::mat4 model = obj.second->get_transform_matrix();
            glm::mat3 normal_matrix = glm::transpose(glm::inverse(model));

            model_constant.model = model;
            model_constant.normal_matrix = normal_matrix;
            vkCmdPushConstants(command_buffer, main_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

            obj.second->draw(command_buffer);

            if (enable_debug_draw) {

                VkDescriptorSet debug_bone_transform_set;
                buffer_info = obj.second->debug_transform_buffer.get_info();
                descriptor_builder::begin(&context->layout_cache, &context->dynamic_descriptor_allocators[context->current_frame])
                    .bind_buffer(0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT).build(debug_bone_transform_set);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_bone_shader->pipeline.layout, 1, 1, &debug_bone_transform_set, 0, NULL);

                model_constant.model = model;
                model_constant.normal_matrix = normal_matrix;
                vkCmdPushConstants(command_buffer, debug_bone_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_bone_shader->pipeline.handle);
                //vkCmdSetDepthTestEnableEXT(command_buffer, false);
                obj.second->draw_debug(command_buffer);
            }
        }
    }
    else {
        if (object_manager.find(single_model_name) != object_manager.end()) {
            const auto& obj = *object_manager.find(single_model_name);
            obj.second->update(delta_time);

            VkDescriptorSet bone_transform_set;
            VkDescriptorBufferInfo buffer_info = obj.second->transform_buffer.get_info();

            descriptor_builder::begin(&context->layout_cache, &context->dynamic_descriptor_allocators[context->current_frame])
                .bind_buffer(0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT).build(bone_transform_set);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.handle);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_shader->pipeline.layout, 1, 1, &bone_transform_set, 0, NULL);

            glm::mat4 model = obj.second->get_transform_matrix();
            glm::mat3 normal_matrix = glm::transpose(glm::inverse(model));

            model_constant.model = model;
            model_constant.normal_matrix = normal_matrix;
            vkCmdPushConstants(command_buffer, main_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

            obj.second->draw(command_buffer);

            if (enable_debug_draw) {

                VkDescriptorSet debug_bone_transform_set;
                buffer_info = obj.second->debug_transform_buffer.get_info();
                descriptor_builder::begin(&context->layout_cache, &context->dynamic_descriptor_allocators[context->current_frame])
                    .bind_buffer(0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT).build(debug_bone_transform_set);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_bone_shader->pipeline.layout, 1, 1, &debug_bone_transform_set, 0, NULL);

                model_constant.model = model;
                model_constant.normal_matrix = normal_matrix;
                vkCmdPushConstants(command_buffer, debug_bone_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_bone_shader->pipeline.handle);
                //vkCmdSetDepthTestEnableEXT(command_buffer, false);
                obj.second->draw_debug(command_buffer);
            }
        }
    }

    if (enable_debug_draw) {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, line_shader->pipeline.handle);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, line_shader->pipeline.layout,
        0, 1, &context->global_data.ubo_data[context->current_frame].descriptor_set, 0, nullptr);

        line_debug_object->draw_debug(command_buffer);
    }

    return true;
}

b8 default_scene::draw_imgui()
{
    i32 frame_count = ImGui::GetFrameCount();

    ImGui::Begin("CS460 Skeletal Animation");
    static const char* current_item = "";
    if (ImGui::BeginTabBar("Tab Bar"))
    {
        if (ImGui::BeginTabItem("General"))
        {
			ImGui::Text("FPS = %f", 1/delta_time);
            ImGui::Spacing();
            ImGui::Separator();

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
			ImGui::Checkbox("Draw a single model", &single_model_draw_mode);
            single_model_name = current_item;

            ImGui::Spacing();
            ImGui::Separator();
            if (current_item != nullptr && current_item != "") {

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

                        if (max != min) {
                            b8 is_changed = ImGui::SliderScalar(obj->animation->mName.C_Str(), ImGuiDataType_::ImGuiDataType_U32, &obj->selected_anim_index, &min, &max);
                            if (is_changed) {
                                obj->set_animation();
                            }
                        }

                        ImGui::SliderFloat("Anim speed", &obj->animation_speed, 0.0f, 2.0f);
                        ImGui::Checkbox("Bone Draw", &enable_debug_draw);
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
    debug_bone_shader->shutdown();
    line_shader->shutdown();

    vulkan_scene::shutdown();

}

b8 default_scene::on_resize(u32 w, u32 h)
{
    return vulkan_scene::on_resize(w, h);
}
