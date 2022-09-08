#include "object.h"

#include <iostream>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

u32 texture_from_file(const char* path, const std::string& directory, bool gamma = false);

object::object(const char* object_name)
{
	position = glm::vec3(0.0f);
	scale = glm::vec3(1.0f);
	rotation_matrix = glm::mat4(1.0f);

	//p_mesh = std::make_unique<mesh>();
	
}

void object::load_model(std::string path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	process_node(scene->mRootNode, scene);
}

void object::process_node(aiNode* node_, const aiScene* scene_)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node_->mNumMeshes; i++)
	{
		aiMesh* mesh = scene_->mMeshes[node_->mMeshes[i]];
		meshes.push_back(process_mesh(mesh, scene_));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node_->mNumChildren; i++)
	{
		process_node(node_->mChildren[i], scene_);
	}
}

mesh object::process_mesh(aiMesh* mesh_, const aiScene* scene_)
{
	std::vector<vertex> vertices;
	std::vector<u32> indices;
	std::vector<texture> textures;

	for (unsigned int i = 0; i < mesh_->mNumVertices; i++)
	{
		vertex vertex_;
		// process vertex positions, normals and texture coordinates

		glm::vec3 vector;
		vector.x = mesh_->mVertices[i].x;
		vector.y = mesh_->mVertices[i].y;
		vector.z = mesh_->mVertices[i].z;
		vertex_.position = vector;

		vector.x = mesh_->mNormals[i].x;
		vector.y = mesh_->mNormals[i].y;
		vector.z = mesh_->mNormals[i].z;
		vertex_.normal = vector;

		if (mesh_->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			vec.x = mesh_->mTextureCoords[0][i].x;
			vec.y = mesh_->mTextureCoords[0][i].y;
			vertex_.uv = vec;
		}
		else
			vertex_.uv = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex_);
	}
	// process indices
	for (unsigned int i = 0; i < mesh_->mNumFaces; i++)
	{
		aiFace face = mesh_->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// process material
	if (mesh_->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene_->mMaterials[mesh_->mMaterialIndex];
		std::vector<texture> diffuseMaps = load_material_textures(material,
			aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<texture> specularMaps = load_material_textures(material,
			aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return {vertices, indices, textures};
}

std::vector<texture> object::load_material_textures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		texture texture_;
		texture_.id = texture_from_file(str.C_Str(), directory);
		texture_.type = typeName;
		texture_.path = str.C_Str();
		textures.push_back(texture_);
	}
	return textures;
}

glm::mat4 object::get_transform_matrix() const
{
	return glm::translate(position) * rotation_matrix * glm::scale(scale);
}

void object::rotate(float degree, glm::vec3 axis)
{
	rotation_matrix = glm::rotate(glm::radians(degree), axis);
}

u32 texture_from_file(const char* path, const std::string& directory, bool gamma)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	u32 textureID;
	glGenTextures(1, &textureID);
	//create texture

	i32 width, height, nrComponents;
	u8* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

/*
i32 get_vertex_component_size(vertex_component component)
{
	switch (component) {
	case vertex_component::E_COLOUR:
		return sizeof(f32) * 4;
	case vertex_component::E_POSITION:
	case vertex_component::E_NORMAL:
	case vertex_component::E_TANGENT:
	case vertex_component::E_BITANGENT:
		return sizeof(f32) * 3;
	case vertex_component::E_UV:
		return sizeof(f32) * 2;
	}
}

i32 get_vertex_component_stride(u32 component_count, vertex_component const* p_components)
{
	i32 stride = 0;

	while (component_count > 0) {
		stride += get_vertex_component_size(*p_components++);
		--component_count;
	}

	return i32();
}
*/
