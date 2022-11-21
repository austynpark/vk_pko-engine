#include "skeleton_node.h"
#include "math/pko_math.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

skeleton_node::skeleton_node(u32 bone_index) : index(bone_index)
{

}

void skeleton_node::add_animation(const std::string& animation_name, const aiNodeAnim* node_anim)
{
	u32 frame_count = node_anim->mNumPositionKeys;

	animation_keyframes anim_frame;
	//anim_frame.frame_count = frame_count;

	anim_frame.pos_count = node_anim->mNumPositionKeys;
	anim_frame.rot_count = node_anim->mNumRotationKeys;
	anim_frame.scale_count = node_anim->mNumScalingKeys;

	anim_frame.pos_frame.resize(node_anim->mNumPositionKeys); 
	anim_frame.rot_frame.resize(node_anim->mNumRotationKeys);
	anim_frame.scale_frame.resize(node_anim->mNumScalingKeys);

	//anim_frame.vqs_frame.resize(frame_count);
	for (u32 i = 0; i < node_anim->mNumPositionKeys; ++i) {
		pko_math::vec3 pos_value;
		memcpy_s(&pos_value, sizeof(pko_math::vec3), &node_anim->mPositionKeys[i].mValue, sizeof(pko_math::vec3));
		anim_frame.pos_frame[i] = {pos_value, (f32)node_anim->mPositionKeys[i].mTime};
		//anim_frame.vqs_frame[i] = { VQS(pos_value, rot_value, scale_value.x), (f32)node_anim->mPositionKeys[i].mTime };
	}

	for (u32 i = 0; i < node_anim->mNumRotationKeys; ++i) {
		pko_math::quat rot_value;
		memcpy_s(&rot_value, sizeof(pko_math::quat), &node_anim->mRotationKeys[i].mValue, sizeof(pko_math::quat));
		anim_frame.rot_frame[i] = { rot_value, (f32)node_anim->mRotationKeys[i].mTime };
	}

	for (u32 i = 0; i < node_anim->mNumScalingKeys; ++i) {
		pko_math::vec3 scale_value;
		memcpy_s(&scale_value, sizeof(pko_math::vec3), &node_anim->mScalingKeys[i].mValue, sizeof(pko_math::vec3));
		anim_frame.scale_frame[i] = { scale_value, (f32)node_anim->mScalingKeys[i].mTime };
	}

	anim_frames[animation_name] = anim_frame;
}

void skeleton_node::interpolate(const std::string& animation_name, f32 time)
{
	f32 delta = 0.0f;
	pko_math::vec3 position;
	pko_math::quat rotation;
	pko_math::vec3 scaling;

	if (anim_frames.find(animation_name) == anim_frames.end())
	{
		return;
	}

	const animation_keyframes& anim_frame = anim_frames[animation_name];

	if (anim_frame.pos_count == 1)
	{
		position = anim_frame.pos_frame[0].position;
	}
	else
	{
		u32 frame_idx = 0;
		for (uint32_t i = 0; i < anim_frame.pos_count - 1; ++i)
		{
			if (time < (f32)anim_frame.pos_frame[i + 1].time)
			{
				frame_idx = i;
				break;
			}
		}

		const keyframe_pos& current_frame = anim_frame.pos_frame[frame_idx];
		const keyframe_pos& next_frame = anim_frame.pos_frame[(frame_idx + 1) % anim_frame.pos_count];

		delta = (time - current_frame.time) / (next_frame.time - current_frame.time);

		position = pko_math::lerp(current_frame.position, next_frame.position, delta);
	}

	
	if (anim_frame.rot_count == 1)
	{
		rotation = anim_frame.rot_frame[0].rotation;
	}
	else
	{
		u32 frame_idx = 0;
		for (uint32_t i = 0; i < anim_frame.rot_count - 1; ++i)
		{
			if (time < (f32)anim_frame.rot_frame[i + 1].time)
			{
				frame_idx = i;
				break;
			}
		}

		const keyframe_rot& current_frame = anim_frame.rot_frame[frame_idx];
		const keyframe_rot& next_frame = anim_frame.rot_frame[(frame_idx + 1) % anim_frame.rot_count];

		delta = (time - current_frame.time) / (next_frame.time - current_frame.time);

		rotation = pko_math::slerp(current_frame.rotation, next_frame.rotation, delta);
	}

	if (anim_frame.scale_count == 1)
	{
		scaling = anim_frame.scale_frame[0].scale;
	}
	else
	{
		u32 frame_idx = 0;
		for (uint32_t i = 0; i < anim_frame.scale_count - 1; ++i)
		{
			if (time < (f32)anim_frame.scale_frame[i + 1].time)
			{
				frame_idx = i;
				break;
			}
		}

		const keyframe_scale& current_frame = anim_frame.scale_frame[frame_idx];
		const keyframe_scale& next_frame = anim_frame.scale_frame[(frame_idx + 1) % anim_frame.scale_count];

		delta = (time - current_frame.time) / (next_frame.time - current_frame.time);

		scaling.x = pko_math::elerp(current_frame.scale.x, next_frame.scale.x, delta);
	}

	local_transformation = VQS{position, rotation, scaling.x};
	//local_transformation =  interpolate_vqs(current_frame.vqs, next_frame.vqs, delta);
}

void skeleton_node::interpolate(const VQS& vqs, f32 time)
{
}

b8 skeleton_node::is_animation_exist(const std::string& animation_name)
{
	if (anim_frames.size() == 0)
		return false;

	if (anim_frames.find(animation_name) == anim_frames.end())
	{
		return false;
	}

	return true;
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
