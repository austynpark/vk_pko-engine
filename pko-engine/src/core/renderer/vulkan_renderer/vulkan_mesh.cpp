#include "vulkan_mesh.h"

#include "vulkan_buffer.h"
#include "vulkan_texture_manager.h"

#include <iostream>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

vulkan_render_object::vulkan_render_object(vulkan_context* context_, const char* path) {
	context = context_;
	position = glm::vec3(0.0f);
	scale = glm::vec3(1.0f);
	rotation = glm::vec3(0.0f);

	load_model(path);
}

vulkan_render_object::vulkan_render_object(vulkan_context* context_, debug_draw_mesh* debug_mesh)
{
	context = context_;
	load_debug_mesh(debug_mesh);
}

void vulkan_render_object::destroy()
{
	vulkan_buffer_destroy(context, &vertex_buffer);
	vulkan_buffer_destroy(context, &index_buffer);
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
	upload_mesh(context, &mesh, &vertex_buffer, &index_buffer);
}

void vulkan_render_object::load_debug_mesh(debug_draw_mesh* object)
{
	line_mesh = *object;
	upload_mesh(context, object, &debug_vertex_buffer, &debug_index_buffer);
}

void vulkan_render_object::process_node(aiNode* node_, const aiScene* scene_)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node_->mNumMeshes; i++)
	{
		aiMesh* p_aimesh = scene_->mMeshes[node_->mMeshes[i]];

		std::vector<vertex> vertices;
		std::vector<u32> indices;
		std::vector<u32> texture_ids;

		for (unsigned int i = 0; i < p_aimesh->mNumVertices; i++)
		{
			vertex vertex_;
			// process vertex positions, normals and texture coordinates

			glm::vec3 vector;
			vector.x = p_aimesh->mVertices[i].x;
			vector.y = p_aimesh->mVertices[i].y;
			vector.z = p_aimesh->mVertices[i].z;
			vertex_.position = vector;

			vector.x = p_aimesh->mNormals[i].x;
			vector.y = p_aimesh->mNormals[i].y;
			vector.z = p_aimesh->mNormals[i].z;
			vertex_.normal = vector;

			if (p_aimesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				vec.x = p_aimesh->mTextureCoords[0][i].x;
				vec.y = p_aimesh->mTextureCoords[0][i].y;
				vertex_.uv = vec;
			}
			else
				vertex_.uv = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex_);
		}
		// process indices
		for (unsigned int i = 0; i < p_aimesh->mNumFaces; i++)
		{
			aiFace face = p_aimesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// process material
		if (p_aimesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene_->mMaterials[p_aimesh->mMaterialIndex];
			std::vector<u32> diffuseMaps = load_material_textures(material,
				aiTextureType_DIFFUSE, "texture_diffuse");
			texture_ids.insert(texture_ids.end(), diffuseMaps.begin(), diffuseMaps.end());
			std::vector<u32> specularMaps = load_material_textures(material,
				aiTextureType_SPECULAR, "texture_specular");
			texture_ids.insert(texture_ids.end(), specularMaps.begin(), specularMaps.end());
		}

		//insert mesh here
		mesh.vertex_offsets.push_back(mesh.vertices.size() * sizeof(vertex));
		mesh.index_offsets.push_back(mesh.indices.size() * sizeof(u32));
		mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());
		mesh.indices.insert(mesh.indices.end(), indices.begin(), indices.end());
		mesh.texture_ids.insert(mesh.texture_ids.end(), texture_ids.begin(), texture_ids.end());
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node_->mNumChildren; i++)
	{
		process_node(node_->mChildren[i], scene_);
	}
}

std::vector<u32> vulkan_render_object::load_material_textures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<u32> textures;
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
		vulkan_texture_manager_add_texture(context, str.C_Str(), texture_);
	}
	return textures;
}

glm::mat4 vulkan_render_object::get_transform_matrix() const
{
	glm::mat4 model(1.0f);
	model *= glm::translate(model, position);
	model *= glm::scale(model, scale);

	glm::quat rotP = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat rotY = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat rotR = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	return model * glm::mat4_cast(rotR) * glm::mat4_cast(rotY) * glm::mat4_cast(rotP);
}

void vulkan_render_object::draw(VkCommandBuffer command_buffer)
{
	u32 mesh_count = mesh.vertex_offsets.size();
	//TODO: bind texture here

	for (u32 i = 0; i < mesh_count; ++i) {
		VkDeviceSize offset = mesh.vertex_offsets[i];
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.handle, &offset);

		offset = mesh.index_offsets[i];
		if (mesh.indices.size() > 0) {
			vkCmdBindIndexBuffer(command_buffer, index_buffer.handle, offset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command_buffer, mesh.indices.size(), 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(command_buffer, mesh.vertices.size(), 1, 0, 0);
		}
	}
}

void vulkan_render_object::draw_debug(VkCommandBuffer command_buffer)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &debug_vertex_buffer.handle, &offset);

	if (bone_debug_mesh.indices.size() > 0) {
		vkCmdBindIndexBuffer(command_buffer, debug_index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(command_buffer, bone_debug_mesh.indices.size(), 1, 0, 0, 0);
	}
	else {
		if (bone_debug_mesh.vertices.size() > 0)
			vkCmdDraw(command_buffer, bone_debug_mesh.vertices.size(), 1, 0, 0);

		if (line_mesh.points.size() > 0)
			vkCmdDraw(command_buffer, line_mesh.points.size(), 1, 0, 0);
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

void upload_mesh(vulkan_context* context, mesh* m, vulkan_allocated_buffer* vertex_buffer, vulkan_allocated_buffer* index_buffer)
{
	vulkan_allocated_buffer staging_buffer;

	vulkan_buffer_create(
		context,
		m->vertices.size() * sizeof(vertex),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		&staging_buffer);

	vulkan_buffer_upload(context, &staging_buffer, m->vertices.data(), m->vertices.size() * sizeof(vertex));

	vulkan_buffer_create(
		context,
		m->vertices.size() * sizeof(vertex),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		vertex_buffer);

	vulkan_buffer_copy(context, &staging_buffer, vertex_buffer, m->vertices.size() * sizeof(vertex));
	vulkan_buffer_destroy(context, &staging_buffer);

	if (m->indices.size() > 0) {

		vulkan_allocated_buffer index_staging_buffer;

		vulkan_buffer_create(
			context,
			m->indices.size() * sizeof(u32),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			&index_staging_buffer);

		vulkan_buffer_upload(context, &index_staging_buffer, m->indices.data(), m->indices.size() * sizeof(u32));

		vulkan_buffer_create(
			context,
			m->indices.size() * sizeof(u32),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			index_buffer
		);

		vulkan_buffer_copy(context, &index_staging_buffer, index_buffer, m->indices.size() * sizeof(u32));
		vulkan_buffer_destroy(context, &index_staging_buffer);
	}

}

void upload_mesh(vulkan_context* context, debug_draw_mesh* m, vulkan_allocated_buffer* vertex_buffer, vulkan_allocated_buffer* index_buffer)
{
	vulkan_allocated_buffer staging_buffer;

	vulkan_buffer_create(
		context,
		m->points.size() * sizeof(glm::vec3),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		&staging_buffer);

	vulkan_buffer_upload(context, &staging_buffer, m->points.data(), m->points.size() * sizeof(glm::vec3));

	vulkan_buffer_create(
		context,
		m->points.size() * sizeof(glm::vec3),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		vertex_buffer);

	vulkan_buffer_copy(context, &staging_buffer, vertex_buffer, m->points.size() * sizeof(glm::vec3));
	vulkan_buffer_destroy(context, &staging_buffer);
}
