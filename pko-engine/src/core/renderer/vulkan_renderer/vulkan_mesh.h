#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_types.inl"

#include "vulkan_image.h"

#include <core/mesh.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <memory>
#include <vector>
#include <string> 


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

void upload_mesh(vulkan_context* context, mesh* m ,vulkan_allocated_buffer* vertex_buffer, vulkan_allocated_buffer* index_buffer);
void upload_mesh(vulkan_context* context, debug_draw_mesh* m, vulkan_allocated_buffer* vertex_buffer, vulkan_allocated_buffer* index_buffer);

//void draw


class vulkan_render_object {
public:
	vulkan_render_object() = default;
	vulkan_render_object(vulkan_context* context, const char* path);
	vulkan_render_object(vulkan_context* context, debug_draw_mesh* debug_mesh);
	virtual void destroy();
	~vulkan_render_object();

	virtual void load_model(std::string path);
	void load_debug_mesh(debug_draw_mesh* object);

	static vertex_input_description get_vertex_input_description();

	vulkan_allocated_buffer vertex_buffer;
	vulkan_allocated_buffer index_buffer;

	vulkan_allocated_buffer debug_vertex_buffer;
	vulkan_allocated_buffer debug_index_buffer;

	glm::mat4 get_transform_matrix() const;

	void draw(VkCommandBuffer command_buffer);
	void draw_debug(VkCommandBuffer command_buffer);

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
	mesh bone_debug_mesh;
	mesh mesh;
	debug_draw_mesh line_mesh;
};

#endif // !VULKAN_MESH_H