#ifndef INVERSE_KINEMATIC_H
#define INVERSE_KINEMATIC_H

#include "defines.h"
#include "skeleton_node.h"

class skinned_mesh;

// CCD (Cyclic-Coordinate Descent) 
/* 
* Returns boolean whether it is possible to reach out Pd (destination point)
* if return value is true, rotation angle according to the joints are passed out
*/
b8 ik_ccd_get_angles(const skinned_mesh* mesh, const glm::vec3& world_Pd, const glm::mat4& model, const i32 depth = 3);

// if first_enter -> store the current_frame of node's
// throught the ik -> get new rotation axis & angle
// by the duration_time -> interpolate node's local_transformation

struct inverse_kinematic
{
	
	// update destination point & recalculate if needed
	b8 update_destination_point(const glm::vec3& world_Pd, const assimp_node* end_effector, const glm::mat4& model, const i32 depth = 3);
	
	void update(f32 animation_speed, f32 dt);

	glm::vec3 destination_point;
	std::vector<VQS> original_frames;
	std::vector<VQS> target_frames;

	f32 running_time;
	f32 duration_time;
};

#endif // !INVERSE_KINEMATIC_H
