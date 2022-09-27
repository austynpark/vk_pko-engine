#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_types.inl"

#include "vulkan_image.h"

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <memory>
#include <vector>
#include <string> 

constexpr u32 MAX_BONES = 64;
constexpr u32 MAX_BONES_PER_VERTEX = 4;

struct model_constant {
	glm::mat4 model;
	glm::mat3 normal_matrix;
	glm::vec3 padding;
};

struct vertex_input_description {
	std::vector<VkVertexInputAttributeDescription> attributes;
	std::vector<VkVertexInputBindingDescription> bindings;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct vertex {
	glm::vec3 position{};
	glm::vec3 normal{};
	glm::vec2 uv{};

#if defined(ANIMATION_ON)
	f32 bone_weight[MAX_BONES_PER_VERTEX]{};
	u32 bone_id[MAX_BONES_PER_VERTEX]{};
#endif

};

struct mesh {
	std::vector<vertex> vertices;
	std::vector<u32> indices;
	std::vector<u32> vertex_offsets;
	std::vector<u32> index_offsets;
	std::vector<u32> texture_ids;

	void upload_mesh(vulkan_context* context, vulkan_allocated_buffer* vertex_buffer, vulkan_allocated_buffer* index_buffer);
};

class vulkan_render_object {
public:
	vulkan_render_object() = default;
	vulkan_render_object(vulkan_context* context, const char* path);
	virtual void destroy();
	~vulkan_render_object();

	virtual void load_model(std::string path);


	static vertex_input_description get_vertex_input_description();

	vulkan_allocated_buffer vertex_buffer;
	vulkan_allocated_buffer index_buffer;

	vulkan_allocated_buffer debug_vertex_buffer;
	vulkan_allocated_buffer debug_index_buffer;

	glm::mat4 get_transform_matrix() const;

	void draw(VkCommandBuffer command_buffer);

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;

protected:
	void process_node(aiNode* node, const aiScene* scene);
	std::vector<u32> load_material_textures(aiMaterial* mat, aiTextureType type,
		std::string typeName);

	vulkan_context* context;

	//TODO: use single mesh for model (only one vertex, index buffer)
	//TODO: every texture will be stored in the combined image sampler (mesh will have id from texture array)
	mesh debug_mesh;
	mesh mesh;
};

#endif // !VULKAN_MESH_H