#ifndef INVERSE_KINEMATIC_H
#define INVERSE_KINEMATIC_H

#include "defines.h"
#include "skeleton_node.h"

// CCD (Cyclic-Coordinate Descent) 
/* 
* Returns boolean whether it is possible to reach out Pd (destination point)
* if return value is true, rotation angle according to the joints are passed out
*/
b8 ik_ccd_get_angles(const assimp_node* end_effector, const glm::vec3& world_Pd/*, std::vector<quat ? >& out_angles*/, const glm::mat4& model, const i32 depth = 3);



#endif // !INVERSE_KINEMATIC_H
