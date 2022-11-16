#include "Inverse_kinematic.h"

#include "math/pko_math.h"
#include <iostream>

b8 ik_ccd_get_angles(const assimp_node* end_effector, const glm::vec3& world_Pd, const glm::mat4& model_matrix, const i32 depth)
{
    if (end_effector->parent == nullptr) {
        std::cout << "no parent(no joint) for ee" << std::endl;

        return false;
    }


    VQS vqs_model_to_world = to_vqs(model_matrix);

    // calculation happens in the local model space
    //const glm::vec3 Pd = glm::vec4(world_Pd, 1.0f);

    i32 node_itr_count = depth;
    
    glm::vec3 Pc = glm::vec3(0.0f); // current_position
    glm::vec3 Pv = glm::vec3(0.0f); // previous_position
    assimp_node* last_joint = nullptr;

    do {
		Pv = Pc;

        const assimp_node* ee = end_effector;
        assimp_node* joint;

        for (i32 i = 0; i < depth; ++i)
        {
            if (ee->parent == nullptr) {
                break;
            }

            joint = ee->parent;
			VQS vqs_bone_to_world = vqs_model_to_world * joint->global_transformation;
			VQS vqs_world_to_bone = (vqs_bone_to_world).inverse();

			VQS vqs_ee_to_world = vqs_model_to_world * end_effector->global_transformation;

			Pc = vqs_world_to_bone.to_matrix() * glm::vec4(vqs_ee_to_world.v.to_vec3(), 1.0f);
            glm::vec3 Pd = vqs_world_to_bone.to_matrix() * glm::vec4(world_Pd, 1.0f);

            if (ee->parent != nullptr) {
                glm::vec3 Ji(0.0f);
                glm::vec3 Pci = glm::normalize(Pc - Ji);
                glm::vec3 Pdi = glm::normalize(Pd - Ji);
                
                float angle = glm::acos(glm::dot(Pci, Pdi));
                if (angle < 0.01f)
                    break;

                glm::vec3 axis = glm::cross(Pci, Pdi);
                
                pko_math::quat rot = pko_math::quat_create();
                rot = pko_math::rotate(rot, angle, axis);
                joint->ik_transformation = VQS({}, rot) * joint->transformation;
 
                ee = ee->parent;
            }

            last_joint = joint;
        }
        
    } while (glm::length(Pc - Pv) > 0.1f);

    return false;
}
