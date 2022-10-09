#include "skeleton_node.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

skeleton_node::skeleton_node(u32 bone_index) : index(bone_index)
{

}

void skeleton_node::add_animation(std::string animation_name, const aiNodeAnim* node_anim)
{
	assert(node_anim->mNumPositionKeys == node_anim->mNumRotationKeys == node_anim->mNumScalingKeys);

	u32 frame_count = node_anim->mNumPositionKeys;

	pos_frame.resize(frame_count); 
	rot_frame.resize(frame_count);
	scale_frame.resize(frame_count);

	vqs_frame.resize(frame_count);

	for (u32 i = 0; i < frame_count; ++i) {
		
		pko_math::vec3 pos_value;
		memcpy_s(&pos_value, sizeof(pko_math::vec3), &node_anim->mPositionKeys[i].mValue, sizeof(pko_math::vec3));
		pko_math::quat rot_value;
		memcpy_s(&rot_value, sizeof(pko_math::quat), &node_anim->mRotationKeys[i].mValue, sizeof(pko_math::quat));
		pko_math::vec3 scale_value;
		memcpy_s(&pos_value, sizeof(pko_math::vec3), &node_anim->mScalingKeys[i].mValue, sizeof(pko_math::vec3));

		pos_frame[i] = {pos_value, (f32)node_anim->mPositionKeys[i].mTime};
		rot_frame[i] = { rot_value, (f32)node_anim->mRotationKeys[i].mTime };
		scale_frame[i] = { scale_value, (f32)node_anim->mScalingKeys[i].mTime };

		vqs_frame[i].vqs = VQS(pos_value, rot_value, scale_value.x);
	}
}

/*
void skeleton_node::set_parent_node(skeleton_node* const parent)
{
	if (parent != nullptr) {
		skeleton_node* node = parent;
		while (node) {
			if (node == this) {
				std::cout << "failed to set parent node" << std::endl;
				return;
			}
			node = node->parent;
		}

		this->parent = parent;
	}

}
*/
