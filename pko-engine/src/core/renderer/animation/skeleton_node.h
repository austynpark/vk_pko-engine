#ifndef SKELETON_NODE_H
#define SKELETON_NODE_H

#include "defines.h"
#include "vqs.h"

#include <vector>
#include <unordered_map>



struct keyframe_pos {
	pko_math::vec3 position;
	f32 time;
};

struct keyframe_rot {
	pko_math::quat rotation;
	f32 time;
};

struct keyframe_scale {
	pko_math::vec3 scale;
	f32 time;
};

struct keyframe_vqs {
	VQS vqs;
	f32 time;
};

class aiNodeAnim;

struct skeleton_node {

	//bone name?
	// key frames?

	// interpolate frame(VQS) to frame(VQS)
	// but how bout iVQS ?? how Im gonna implement it into skeleton_node
	// first, i want to leave iVQS as an available option that can be turn on/off at anytime
	// second, to do that, there are several ways that comes to my mind
	// get the target frame (which would probably be 60fps), for my sake, i'll just fix that to 60fps
	// I can precompute all of the frames and store it but, that would be waste of memory space so instead, I would just
	// compute on-fly and just store the step value here

	skeleton_node(skeleton_node* parent, u32 bone_index);
	void add_animation(std::string animation_name, const aiNodeAnim* node_anim);

	glm::mat4 offset;
	glm::mat4 final_transformation;

	//TODO: support multiple animation
	std::vector<keyframe_vqs> vqs_frame;

	std::vector<keyframe_pos> pos_frame;
	std::vector<keyframe_rot> rot_frame;
	std::vector<keyframe_scale> scale_frame;

	std::vector<skeleton_node*> childern;
	skeleton_node* parent = nullptr;

	u32 bone_idx;
};

#endif // !SKELETON_NODE_H

