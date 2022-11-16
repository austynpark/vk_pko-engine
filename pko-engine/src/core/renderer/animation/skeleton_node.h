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

	skeleton_node(u32 bone_index);
	void add_animation(std::string animation_name, const aiNodeAnim* node_anim);
	//void set_parent_node(skeleton_node* const parent);

	aiMatrix4x4 offset_mat;
	VQS offset;
	VQS final_transformation;

	//TODO: support multiple animation
	std::vector<keyframe_vqs> vqs_frame;

	std::vector<keyframe_pos> pos_frame;
	std::vector<keyframe_rot> rot_frame;
	std::vector<keyframe_scale> scale_frame;

	//bone_index
	u32 index;

	// bone length (local length)
	u32 length = 0; // length to parent joint
};

// convert aiNode to custom struct (convert data to VQS)
struct assimp_node { // actual joint

	std::string name;
	VQS transformation; // local_node transformation
	VQS global_transformation; // parent_transform * node_transform
	VQS ik_transformation;

	std::vector<assimp_node*> children;
	u32 childern_num = 0;
	assimp_node* parent = nullptr;
	skeleton_node* bone = nullptr;
};

#endif // !SKELETON_NODE_H

