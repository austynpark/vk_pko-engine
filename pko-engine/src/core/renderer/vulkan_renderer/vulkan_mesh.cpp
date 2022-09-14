#include "vulkan_mesh.h"

#include "vulkan_buffer.h"

#include <iostream>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

vulkan_render_object::vulkan_render_object(vulkan_context* context_, const char* path) {
	context = context_;
	position = glm::vec3(0.0f);
	scale = glm::vec3(1.0f);
	rotation = glm::vec3(0.0f);

	load_model(path);
}

void vulkan_render_object::upload_mesh()
{
	u32 mesh_count = meshes.size();

	vertex_buffers.resize(mesh_count);
	index_buffers.resize(mesh_count);

	for (u32 i = 0; i < mesh_count; ++i) {

		vulkan_allocated_buffer staging_buffer;

		vulkan_buffer_create(
			context,
			meshes[i].vertices.size() * sizeof(vertex),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			&staging_buffer);

		void* data;
		VK_CHECK(vmaMapMemory(context->vma_allocator, staging_buffer.allocation, &data));

		memcpy(data, meshes[i].vertices.data(), meshes[i].vertices.size() * sizeof(vertex));

		vmaUnmapMemory(context->vma_allocator, staging_buffer.allocation);

		vulkan_buffer_create(
			context,
			meshes[i].vertices.size() * sizeof(vertex),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			&vertex_buffers[i]);

		vulkan_buffer_copy(context, &staging_buffer, &vertex_buffers[i], meshes[i].vertices.size() * sizeof(vertex));
		vulkan_buffer_destroy(context, &staging_buffer);


		if (meshes[i].indices.size() > 0) {

			vulkan_allocated_buffer index_staging_buffer;

			vulkan_buffer_create(
				context,
				meshes[i].indices.size() * sizeof(u32),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_AUTO,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				&index_staging_buffer);

			VK_CHECK(vmaMapMemory(context->vma_allocator, index_staging_buffer.allocation, &data));

			memcpy(data, meshes[i].indices.data(), meshes[i].indices.size() * sizeof(u32));

			vmaUnmapMemory(context->vma_allocator, index_staging_buffer.allocation);

			vulkan_buffer_create(
				context,
				meshes[i].indices.size() * sizeof(u32),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_AUTO,
				VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
				&index_buffers[i]
			);

			vulkan_buffer_copy(context, &index_staging_buffer, &index_buffers[i], meshes[i].indices.size() * sizeof(u32));
			vulkan_buffer_destroy(context, &index_staging_buffer);
		}
	}
}

void vulkan_render_object::vulkan_render_object_destroy()
{
	for (auto& mesh : meshes) {
		for (auto& texture : mesh.textures)
			vulkan_texture_destroy(context, &texture);
	}

	for (auto& vertex_buffer : vertex_buffers)
		vmaDestroyBuffer(context->vma_allocator, vertex_buffer.handle, vertex_buffer.allocation);

	for(auto& index_buffer: index_buffers)
		vmaDestroyBuffer(context->vma_allocator, index_buffer.handle, index_buffer.allocation);
}

vulkan_render_object::~vulkan_render_object()
{
}

void vulkan_render_object::load_model(std::string path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	//directory = path.substr(0, path.find_last_of('/'));

	process_node(scene->mRootNode, scene);

}

void vulkan_render_object::process_node(aiNode* node_, const aiScene* scene_)
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

mesh vulkan_render_object::process_mesh(aiMesh* mesh_, const aiScene* scene_)
{
	std::vector<vertex> vertices;
	std::vector<u32> indices;
	std::vector<vulkan_texture> textures;

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
		std::vector<vulkan_texture> diffuseMaps = load_material_textures(material,
			aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<vulkan_texture> specularMaps = load_material_textures(material,
			aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return { vertices, indices, textures };
}

std::vector<vulkan_texture> vulkan_render_object::load_material_textures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<vulkan_texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		vulkan_image image;
		std::string directory = "model/";
		directory.append(str.C_Str());
		load_image_from_file(context, directory.c_str(), &image);

		vulkan_texture texture_;
		vulkan_texture_create(context, &texture_, image, VK_FILTER_LINEAR);
		textures.push_back(texture_);
	}
	return textures;
}

glm::mat4 vulkan_render_object::get_transform_matrix() const
{
	glm::mat4 model(1.0f);
	model = glm::translate(model, position);
	model = glm::scale(model, scale);

	glm::quat rotP = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat rotY = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat rotR = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	return model * glm::mat4_cast(rotR) * glm::mat4_cast(rotY) * glm::mat4_cast(rotP);
}

void vulkan_render_object::rotate(float degree, glm::vec3 axis)
{
	//rotation_matrix = glm::rotate(glm::radians(degree), axis);
}

void vulkan_render_object::draw(VkCommandBuffer command_buffer)
{
	u32 mesh_count = meshes.size();

	for (u32 i = 0; i < mesh_count; ++i) {

		for (u32 j = 0; j < meshes[i].textures.size(); ++j) {

			//meshes[i].textures[j]
		}

		VkDeviceSize offset = 0 ;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers[i].handle, &offset);
		
		if (meshes[i].indices.size() > 0) {
			vkCmdBindIndexBuffer(command_buffer, index_buffers[i].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command_buffer, meshes[i].indices.size(), 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(command_buffer, meshes[i].vertices.size(), 1, 0, 0);
		}
	}
}

vertex_input_description vulkan_render_object::get_vertex_input_description()
{
	vertex_input_description result;

	VkVertexInputBindingDescription input_binding_description;
	input_binding_description.binding = 0;
	input_binding_description.stride = sizeof(vertex);
	input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	result.bindings.push_back(input_binding_description);

	std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions(3);

	input_attribute_descriptions[0].binding = 0;
	input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	input_attribute_descriptions[0].location = 0;
	input_attribute_descriptions[0].offset = offsetof(vertex, position);

	input_attribute_descriptions[1].binding = 0;
	input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	input_attribute_descriptions[1].location = 1;
	input_attribute_descriptions[1].offset = offsetof(vertex, normal);

	input_attribute_descriptions[2].binding = 0;
	input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	input_attribute_descriptions[2].location = 2;
	input_attribute_descriptions[2].offset = offsetof(vertex, uv);

	result.attributes = input_attribute_descriptions;

	return result;
}
