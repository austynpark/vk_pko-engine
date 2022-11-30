#include "Inverse_kinematic.h"

#include "math/pko_math.h"
#include "core/renderer/vulkan_renderer/vulkan_skinned_mesh.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <iostream>

VQS get_global_transformation(const std::vector<assimp_node*>& route, const assimp_node* node)
{
    u32 route_count = route.size();

    glm::mat4 mat(1.0f);

    for (u32 i = 0; i < route_count; ++i) {

        if (route[i]->bone != nullptr)
            mat = mat * route[i]->bone->local_transformation.to_matrix();
        else
            mat = mat * route[i]->bind_pose_transformation.to_matrix();
        
        if (route[i]->name == node->name)
            break;
    }

    return to_vqs(mat);
}


b8 ik_ccd_get_angles(const skinned_mesh* mesh, const glm::vec3& world_Pd, const glm::mat4& model_matrix, const i32 depth)
{
    if (mesh->get_end_effector()->parent == nullptr) {
        std::cout << "no parent(no joint) for ee" << std::endl;

        return false;
    }

    VQS vqs_model_to_world = to_vqs(model_matrix);

    const assimp_node* end_effector = mesh->get_end_effector();

    // calculation happens in the local model space
    //const glm::vec3 Pd = glm::vec4(world_Pd, 1.0f);    

    //VQS joint_global_transformation = joint->global_transformation;
    glm::vec3 Pc(0.0f);
    glm::vec3 Pv(FLT_MAX);

     do {
		Pv = Pc;
        
		assimp_node* last_joint = mesh->end_effectors[mesh->selected_ee_idx];
		const assimp_node* ee = end_effector;
		assimp_node* joint = end_effector->parent;

        u32 bone_itr = 0;
        while (bone_itr++ < depth)
        {
            if (joint == nullptr) {
                break;
            }

            if (joint->bone == nullptr) {
				last_joint = joint;
				joint = joint->parent;
                std::cout << "no bone on the joint!!" << std::endl;
                continue;
            }

			VQS vqs_ee_to_world = vqs_model_to_world * mesh->global_inverse_transform * get_global_transformation(mesh->route_to_ee[mesh->selected_ee_idx], end_effector);

			VQS vqs_bone_to_world = vqs_model_to_world * mesh->global_inverse_transform * joint->global_transformation;
			VQS vqs_world_to_bone = (vqs_bone_to_world).inverse();

            // unit test check assert(Pc == Pc2)
            //pko_math::vec3 test = (vqs_ee_to_world * pko_math::vec3(0.0f));
            //glm::vec3 Pc2 = (vqs_world_to_bone* test).to_vec3();
            //Pc2 = vqs_world_to_bone.to_matrix() * vqs_ee_to_world.to_matrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			//Pc = vqs_world_to_bone.to_matrix() * glm::vec4(vqs_ee_to_world.v.to_vec3(), 1.0f); // current_position

			// local position of the end effector
			Pc = vqs_world_to_bone.to_matrix() * glm::vec4(vqs_ee_to_world.v.to_vec3(), 1.0f);
            //Pc = (vqs_world_to_bone * vqs_ee_to_world.v.to_vec3()).to_vec3();
            // local position of the destination point
            glm::vec3 Pd = vqs_world_to_bone.to_matrix() * glm::vec4(world_Pd, 1.0f);

            glm::vec3 destination = vqs_bone_to_world.to_matrix() * glm::vec4(Pd, 1.0f);
            glm::vec3 current_ee_world_pos = vqs_bone_to_world.to_matrix() * glm::vec4(Pc, 1.0f);
                 
            //Pd = (vqs_world_to_bone * world_Pd).to_vec3();

            if (ee->parent != nullptr) {
                glm::vec3 Ji = glm::vec3(0.0f, 0.0f, 0.0f);
                glm::vec3 Pci = glm::normalize(Pc - Ji);
                glm::vec3 Pdi = glm::normalize(Pd - Ji);
                
                if (glm::all(glm::isnan(Pci)) || glm::all(glm::isnan(Pdi))) {
					last_joint = joint;
					joint = joint->parent;
                    //std::cout << joint->bone->name <<" Pci Pdi NAN ANANANANA!!" << std::endl;
                    continue;
                }


                float PcPd_dot = glm::dot(Pci, Pdi);
                float angle = glm::acos(PcPd_dot);

                //std::cout << "angle: " << glm::degrees(angle) << std::endl;

                if (PcPd_dot >= 1) {
					last_joint = joint;
					joint = joint->parent;

                    //std::cout << "never happen" << std::endl;
                    break;
                }

                if (glm::degrees(angle) < 5.0f) {

					last_joint = joint;
					joint = joint->parent;


                    //std::cout << joint->bone->name <<" angle below 5 degree!!" << std::endl;
                    continue;
				}
    
                if (glm::distance(Pc, Pd) <= 0.1f)
                {
                    //std::cout << "close enough!!!" << std::endl;
                    return false;
                }
          
                glm::vec3 axis = glm::normalize(glm::cross(Pci, Pdi));
                glm::mat4 rot_mat = glm::rotate(angle, axis);

                //remove old local transformation from a global transformation
                joint->global_transformation = joint->global_transformation * joint->bone->local_transformation.inverse();
                // renew local transformation with extra rotation
                joint->bone->local_transformation = joint->bone->local_transformation * VQS{{}, pko_math::to_quat(rot_mat), 1.0f};
                // update the global transformation with the new local transformation
                joint->global_transformation = joint->global_transformation * joint->bone->local_transformation;
            }

            last_joint = joint;
            joint = joint->parent;
        }
        
    } while (glm::distance(Pc, Pv) > 1.0f);

    
	//joint->ik_transformation = VQS({}, rot) * joint->transformation;

    // duration_time, animation_speed, dt, running_time
    // running_time += animation_speed * dt;
    // animation_time = fmod(running_time, duration_time)
    // delta = 

    return false;
}

b8 inverse_kinematic::update_destination_point(const glm::vec3& world_Pd, const assimp_node* end_effector, const glm::mat4& model, const i32 depth)
{


    return b8();
}

void inverse_kinematic::update(f32 animation_speed, f32 dt)
{
}
