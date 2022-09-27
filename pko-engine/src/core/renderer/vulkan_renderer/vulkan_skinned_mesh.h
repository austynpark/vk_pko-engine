#ifndef VULKAN_SKINNED_MESH
#define VULKAN_SKINNED_MESH

#include "defines.h"

#include "vulkan_mesh.h"

#include <unordered_map>

struct vertex_bone_data {
	std::vector<u32> ids;
	std::vector<f32> weights;

	void add(u32 bone_id, f32 weight) {
		ids.push_back(bone_id);
		weights.push_back(weight);
	}
};

struct bone_info {
	aiMatrix4x4 offset;
	aiMatrix4x4 final_transformation;

	bone_info() {
		offset = aiMatrix4x4();
		final_transformation = aiMatrix4x4();
	}
};

class skinned_mesh : public vulkan_render_object {
public:
	skinned_mesh(vulkan_context* context, const char* file_name);
	~skinned_mesh();

	void load_model(std::string path) override;
	void draw(VkCommandBuffer command_buffer);
	void draw_debug(VkCommandBuffer command_buffer);
	void update(f32 dt);
	void destroy() override;

	std::vector<bone_info> m_bone_info;
	//store bone id
	std::unordered_map<std::string, u32> bone_mapping;
	//per-vertex bone info
	std::vector<vertex_bone_data> vertex_bone;

	std::vector<glm::mat4> bone_transforms;
	vulkan_allocated_buffer transform_buffer;

	static vertex_input_description get_vertex_input_description();
	
	void set_animation();

	const aiScene* scene;
	aiAnimation* animation;
	f32 animation_speed = 0.75f;
	uint32_t animation_count;
	uint32_t selected_anim_index = 0;
	b8 enable_debug_draw = true;
private:
	Assimp::Importer importer;

	aiMatrix4x4 global_inverse_transform;
	void load_bones(aiMesh* mesh, u32 vertex_offset);

	// Find animation for a given node
	const aiNodeAnim* find_node_anim(const aiAnimation* animation, const std::string node_name);

	// Get node hierarchy for current animation time
	void read_node_hierarchy(float animation_time, const aiNode* node, const aiMatrix4x4& parent_transform);

	// iterate through the bones and set vertex for the bone drawing
	void process_bone_vertex(const aiNode* node);

	// Returns a 4x4 matrix with interpolated translation between current and next frame
	aiMatrix4x4 interpolate_translation(float time, const aiNodeAnim* node_anim)
	{
		aiVector3D translation;

		if (node_anim->mNumPositionKeys == 1)
		{
			translation = node_anim->mPositionKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < node_anim->mNumPositionKeys - 1; i++)
			{
				if (time < (float)node_anim->mPositionKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = node_anim->mPositionKeys[frameIndex];
			aiVectorKey nextFrame = node_anim->mPositionKeys[(frameIndex + 1) % node_anim->mNumPositionKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			translation = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Translation(translation, mat);
		return mat;
	}

	// Returns a 4x4 matrix with interpolated rotation between current and next frame
	aiMatrix4x4 interpolate_rotation(float time, const aiNodeAnim* node_anim)
	{
		aiQuaternion rotation;

		if (node_anim->mNumRotationKeys == 1)
		{
			rotation = node_anim->mRotationKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < node_anim->mNumRotationKeys - 1; i++)
			{
				if (time < (float)node_anim->mRotationKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiQuatKey currentFrame = node_anim->mRotationKeys[frameIndex];
			aiQuatKey nextFrame = node_anim->mRotationKeys[(frameIndex + 1) % node_anim->mNumRotationKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiQuaternion& start = currentFrame.mValue;
			const aiQuaternion& end = nextFrame.mValue;

			aiQuaternion::Interpolate(rotation, start, end, delta);
			rotation.Normalize();
		}

		aiMatrix4x4 mat(rotation.GetMatrix());
		return mat;
	}

	// Returns a 4x4 matrix with interpolated scaling between current and next frame
	aiMatrix4x4 interpolate_scale(float time, const aiNodeAnim* node_anim)
	{
		aiVector3D scale;

		if (node_anim->mNumScalingKeys == 1)
		{
			scale = node_anim->mScalingKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < node_anim->mNumScalingKeys - 1; i++)
			{
				if (time < (float)node_anim->mScalingKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = node_anim->mScalingKeys[frameIndex];
			aiVectorKey nextFrame = node_anim->mScalingKeys[(frameIndex + 1) % node_anim->mNumScalingKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			scale = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Scaling(scale, mat);
		return mat;
	}

};

#endif // !VULKAN_SKINNED_MESH
