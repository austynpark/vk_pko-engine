#include "default_scene.h"

#include "../vulkan_shader.h"
#include "../vulkan_mesh.h"
#include "../vulkan_skinned_mesh.h"
#include "../vulkan_pipeline.h"

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
    non_animation_shader = std::make_unique<vulkan_shader>(api_context, &context->main_renderpass);

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

    if (line_shader->init(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, false, &line_input) != true) {
        std::cout << "line_shader init fail" << std::endl;
        return false;
    }

    //debug_mesh_add_line(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), &path);
    //debug_mesh_add_line(glm::vec3(-1.0f, -1.0f, 0.0f), &path);
    //debug_mesh_add_line(glm::vec3(1.0f, -1.0f, 0.0f), &path);
    //debug_mesh_link_begin_end(&path);

    {
        path_builder_initialize_bezier_curve(
            glm::vec3(-1, 0, -1),
            glm::vec3(0, 0, -1),
            glm::vec3(1, 0, 0),
            glm::vec3(2, 0, 0),
            &path_ease_inout
        );

        path_builder_add_control_points(
            glm::vec3(3, 0, -1),
            glm::vec3(2, 0, -2),
            glm::vec3(1, 0, -5),
            &path_ease_inout
        );

		path_builder_add_control_points(
            glm::vec3(0, 0, -4),
            glm::vec3(-0.5f, 0, -3),
            glm::vec3(-2, 0, -1),
            &path_ease_inout
        );

        path_builder_add_time_velocity_stamp(
            0.3f,
            1.0f,
            &path_ease_inout
        );

        path_builder_add_time_velocity_stamp(
            0.8f,
            1.0f,
            &path_ease_inout
        );

        path_builder_add_time_velocity_stamp(
            1.0f,
            0.0f,
            &path_ease_inout
        );

        path_build(&path_ease_inout);
    }

    line_debug_object = std::make_unique<vulkan_render_object>(context, &path_ease_inout.line_mesh);

    main_shader->add_stage("animation.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("animation.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (main_shader->init() != true)
    {
        std::cout << "main_shader init fail" << std::endl;
        return false;
    }

	non_animation_shader->add_stage("test.vert", VK_SHADER_STAGE_VERTEX_BIT)
        .add_stage("test.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    if (non_animation_shader->init() != true)
    {
        std::cout << "non_animation_shader init fail" << std::endl;
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

    if (animate_along_path) // update u
        path_update(delta_time, &path_ease_inout);

	model_constant model_constant{};

	//NOTE: non-animation-object rendering test code
	vulkan_pipeline_bind(&graphics_commands.at(context->current_frame), VK_PIPELINE_BIND_POINT_GRAPHICS, &non_animation_shader->pipeline);
	model_constant.model = test_object->get_transform_matrix();
	vkCmdPushConstants(command_buffer, non_animation_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model_constant), &model_constant);
	test_object->draw(command_buffer);

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

            glm::mat4 model;
            glm::mat3 normal_matrix = glm::transpose(glm::inverse(model));

			if (animate_along_path) {
                if (use_ease_inout_path) {
                    model = path_get_along(delta_time, &path_ease_inout, obj.second.get());
                }
             }
            else {
                model = obj.second->get_transform_matrix();
            }

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

    ImGui::Begin("CS460 Skeletal Animation", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
    static const char* current_item = "";
    static const char* current_end_effector = "";
    if (ImGui::BeginTabBar("Tab Bar"))
    {
        if (ImGui::BeginTabItem("General"))
        {
			ImGui::Text("FPS = %f", 1/delta_time);
            ImGui::Spacing();
            ImGui::Separator();

			if (ImGui::CollapsingHeader("Path"))
			{
				ImGui::Checkbox("Animate along path", &animate_along_path);
                ImGui::Checkbox("Inverse Kinematic", &use_inverse_kinematic);
				//ImGui::Checkbox("Use Three velocity-time path", &use_ease_inout_path);
			}

            if (!animate_along_path) {
                if (ImGui::BeginCombo("Object", current_item))
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
            }
            else { // animate along the path
                single_model_name = "boxguy";
                object_manager["boxguy"]->selected_anim_index = 12;
                object_manager["boxguy"]->set_animation();

				if (ImGui::BeginCombo("End effector", current_end_effector))
                {
                    u32 i = 0;
                    for (const auto& ee : object_manager["boxguy"]->end_effectors)
                    {
                        bool is_selected =
                            !strcmp(current_end_effector,
                                ee->name.c_str()); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(ee->name.c_str(), is_selected))
                            current_end_effector = ee->name.c_str();
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                            object_manager["boxguy"]->selected_ee_idx = i;
                        }

                        ++i;
                    }

                    // object combo
                    ImGui::EndCombo();
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
    line_debug_object->destroy();

    //TODO: erase test_object & shader
    test_object->destroy();
    non_animation_shader->shutdown();

    vulkan_scene::shutdown();

}

b8 default_scene::on_resize(u32 w, u32 h)
{
    return vulkan_scene::on_resize(w, h);
}
