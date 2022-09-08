#ifndef OBJECT_H
#define OBJECT_H

#include "defines.h"
/*
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <memory>
#include <vector>
#include <string>

/*
enum class vertex_component {
	E_POSITION,
	E_NORMAL,
	E_UV,
	E_COLOUR,
	E_TANGENT,
	E_BITANGENT
};

i32 get_vertex_component_size(vertex_component component);
i32 get_vertex_component_stride(u32 component_count, vertex_component const* p_components);

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct texture {
	u32 id;
	std::string type;
	std::string path;
};

struct mesh {
	std::vector<vertex> vertices;
	std::vector<u32> indices;
	std::vector<texture> textures;
	//TODO: std::vector<texture> ???
	glm::mat4 transform_matrix;
};

class object {
	
public:
	object() = delete;
	virtual ~object() = 0 {};
	object(const char* object_name);

	void load_model(std::string path);
	void process_node(aiNode* node, const aiScene* scene);
	mesh process_mesh(aiMesh* mesh, const aiScene* scene);
	std::vector<texture> load_material_textures(aiMaterial* mat, aiTextureType type,
		std::string typeName);


	//std::unique_ptr<mesh> p_mesh;

	glm::mat4 get_transform_matrix() const;
	void rotate(float degree, glm::vec3 axis);

	glm::vec3 position;
	glm::vec3 scale;
	glm::mat4 rotation_matrix;
	
	// rotation (quat)
};

*/


#endif // !OBJECT_H
