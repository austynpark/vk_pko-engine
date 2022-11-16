#ifndef VULKAN_SKINNED_MESH
#define VULKAN_SKINNED_MESH

#include "defines.h"

#include "vulkan_mesh.h"
#include "core/renderer/animation/skeleton_node.h"

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
	void update(f32 dt, b8 ik_update);
	void destroy() override;

	std::vector<skeleton_node*> m_bone_info;
	std::vector<assimp_node*> end_effectors;
	u32 selected_ee_idx = 0;

	//store bone id
	std::unordered_map<std::string, u32> bone_mapping;
	//per-vertex bone info
	std::vector<vertex_bone_data> vertex_bone;

	std::vector<glm::mat4> bone_transforms;
	std::vector<glm::mat4> debug_bone_transforms;
	vulkan_allocated_buffer transform_buffer;
	vulkan_allocated_buffer debug_transform_buffer;

	static vertex_input_description get_vertex_input_description();

	void set_animation();
	const assimp_node* get_end_effector() const;
	void set_inverse_kinematic_transformation(const i32 ik_depth);

	const aiScene* scene;
	aiAnimation* animation;
	f32 animation_speed = 0.75f;
	uint32_t animation_count;
	uint32_t selected_anim_index = 0;

	assimp_node* assimp_node_root;
private:
	Assimp::Importer importer;
	f32 running_time = 0.0f;

	VQS global_inverse_transform;
	void load_bones(aiMesh* mesh, u32 vertex_offset);

	// Find animation for a given node
	const aiNodeAnim* find_node_anim(const aiAnimation* animation, const std::string node_name);

	VQS interpolate(f32 time, const aiNodeAnim* node_anim);

	// Get node hierarchy for current animation time
	void read_node_hierarchy(float animation_time, assimp_node* node, const VQS& parent_transform, b8 ik_update);

	// iterate through the bones and set vertex for the bone drawing
	void process_bone_vertex(const aiNode* ai_node, assimp_node* node);

};

#endif // !VULKAN_SKINNED_MESH
