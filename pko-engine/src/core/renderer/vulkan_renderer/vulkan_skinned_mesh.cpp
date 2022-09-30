#include "vulkan_skinned_mesh.h"

#include "vulkan_buffer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

skinned_mesh::skinned_mesh(vulkan_context* context, const char* file_name) : vulkan_render_object() {
	this->context = context;
	position = glm::vec3(0.0f);
	scale = glm::vec3(1.0f);
	rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	animation_count = 0;
	selected_anim_index = 0;

	load_model(file_name);
}

skinned_mesh::~skinned_mesh()
{
}

void skinned_mesh::load_model(std::string path)
{
	scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	// set pAnimation
	animation_count = scene->mNumAnimations;
	set_animation();
	// use single vertex buffer that including sub-meshes
	u32 vertex_count = 0;
	for (u32 i = 0; i < scene->mNumMeshes; ++i) {
		vertex_count += scene->mMeshes[i]->mNumVertices;
	}

	vertex_bone.resize(vertex_count);
	global_inverse_transform = scene->mRootNode->mTransformation;
	global_inverse_transform.Inverse();

	u32 vertex_offset = 0;
	for (u32 i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh* p_aiMesh = scene->mMeshes[i];
		if (p_aiMesh->HasBones()) {
			load_bones(p_aiMesh, vertex_offset);
		}
		vertex_offset += scene->mMeshes[i]->mNumVertices;
	}

	bone_transforms.resize(m_bone_info.size(), glm::mat4(1.0f));
	debug_bone_transforms.resize(m_bone_info.size(), glm::mat4(1.0f));
	
	vertex_offset = 0;
	for (u32 i = 0; i < scene->mNumMeshes; ++i) {
		for (u32 v = 0; v < scene->mMeshes[i]->mNumVertices; ++v) {
			vertex vertex;
			//vertex.position
			vertex.position = glm::make_vec3(&scene->mMeshes[i]->mVertices[v].x);
			
			if (scene->mMeshes[i]->HasNormals())
				vertex.normal = glm::make_vec3(&scene->mMeshes[i]->mNormals[v].x);
			
			if (scene->mMeshes[i]->HasTextureCoords(v))
				vertex.uv = glm::vec2(scene->mMeshes[i]->mTextureCoords[v]->x, scene->mMeshes[i]->mTextureCoords[v]->y);
	
			//assert(vertex_bone[vertex_offset + v].weights.size() <= MAX_BONES_PER_VERTEX);
			u32 bone_count = vertex_bone[vertex_offset + v].weights.size() <= MAX_BONES_PER_VERTEX ? vertex_bone[vertex_offset + v].weights.size() : MAX_BONES_PER_VERTEX;
			for (u32 j = 0; j < bone_count; ++j) {
				vertex.bone_weight[j] = vertex_bone[vertex_offset + v].weights[j];
				vertex.bone_id[j] = vertex_bone[vertex_offset + v].ids[j];
			}

			/*
			if (scene->mMeshes[i]->mMaterialIndex >= 0)
			{
				aiMaterial* material = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
				std::vector<u32> diffuseMaps = load_material_textures(material,
					aiTextureType_DIFFUSE, "texture_diffuse");
				mesh.texture_ids.insert(mesh.texture_ids.end(), diffuseMaps.begin(), diffuseMaps.end());
				std::vector<u32> specularMaps = load_material_textures(material,
					aiTextureType_SPECULAR, "texture_specular");
				mesh.texture_ids.insert(mesh.texture_ids.end(), specularMaps.begin(), specularMaps.end());
			}
			*/

			mesh.vertices.push_back(vertex);
		}
		vertex_offset += scene->mMeshes[i]->mNumVertices;
	}
	
	for (u32 i = 0; i < scene->mNumMeshes; ++i) {
		u32 index_offset = mesh.indices.size();
		for (u32 j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
			mesh.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[0] + index_offset);
			mesh.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[1] + index_offset);
			mesh.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[2] + index_offset);
		}
	}

	aiNode* node = scene->mRootNode;
	process_bone_vertex(node);

	debug_mesh.upload_mesh(context, &debug_vertex_buffer, &debug_index_buffer);
	mesh.upload_mesh(context, &vertex_buffer, &index_buffer);
	u32 ubo_size = vulkan_uniform_buffer_pad_size(context->device_context.properties.limits.minUniformBufferOffsetAlignment, MAX_BONES * sizeof(glm::mat4));

	vulkan_buffer_create(
		context,
		ubo_size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		/*VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE |*/ VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		&transform_buffer);

	vulkan_buffer_upload(context, &transform_buffer, bone_transforms.data(), ubo_size);

	vulkan_buffer_create(
		context,
		ubo_size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		/*VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE |*/ VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		&debug_transform_buffer);

	vulkan_buffer_upload(context, &debug_transform_buffer, debug_bone_transforms.data(), ubo_size);
}

void skinned_mesh::load_bones(aiMesh* mesh, u32 vertex_offset)
{

	for (u32 i = 0; i < mesh->mNumBones; ++i) {
		u32 index = 0;
		std::string name(mesh->mBones[i]->mName.data);

		// bone already exists in map
		if (bone_mapping.find(name) != bone_mapping.end()) {
			index = bone_mapping[name];
		}
		else { // add new bone to map
			index = m_bone_info.size();

			bone_info bone;
			bone.offset = mesh->mBones[i]->mOffsetMatrix;
			m_bone_info.push_back(bone);

			bone_mapping[name] = index;
		}

		for (u32 j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
			// since we are using a single vertex buffer
			// vertex_offset + bones::weights::vertex_id would be actuall vertex index in the 'bones'
			u32 vertex_id = vertex_offset + mesh->mBones[i]->mWeights[j].mVertexId;
			vertex_bone[vertex_id].add(index, mesh->mBones[i]->mWeights[j].mWeight);
		}
	}
}


void skinned_mesh::process_bone_vertex(const aiNode* node) {

	vertex vert{};
	vert.bone_weight[0] = 1.0f;
	vert.position = glm::vec3(0.0f);

	if (node->mParent != nullptr) {

		const auto &parent_bone_itr = bone_mapping.find(node->mParent->mName.C_Str());
		const auto& child_bone_itr = bone_mapping.find(node->mName.C_Str());

		if(parent_bone_itr != bone_mapping.end() && child_bone_itr != bone_mapping.end()) {
			vert.bone_id[0] = parent_bone_itr->second;
			debug_mesh.vertices.push_back(vert);
			vert.bone_id[0] = child_bone_itr->second;
			debug_mesh.vertices.push_back(vert);
		}
	}

	if (node->mNumChildren == 0) {
		return;
	}

	for (u32 i = 0; i < node->mNumChildren; ++i) {
		process_bone_vertex(node->mChildren[i]);
	}
}

void skinned_mesh::draw(VkCommandBuffer command_buffer)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.handle, &offset);

	if (mesh.indices.size() > 0) {
		vkCmdBindIndexBuffer(command_buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(command_buffer, mesh.indices.size(), 1, 0, 0, 0);
	}
	else {
		vkCmdDraw(command_buffer, mesh.vertices.size(), 1, 0, 0);
	}
}

void skinned_mesh::draw_debug(VkCommandBuffer command_buffer)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &debug_vertex_buffer.handle, &offset);

	if (debug_mesh.indices.size() > 0) {
		vkCmdBindIndexBuffer(command_buffer, debug_index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(command_buffer, debug_mesh.indices.size(), 1, 0, 0, 0);
	}
	else {
		vkCmdDraw(command_buffer, debug_mesh.vertices.size(), 1, 0, 0);
	}
}

void skinned_mesh::update(f32 dt)
{
	float ticks_per_second = (f32)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
	float time_in_ticks = dt * ticks_per_second;
	float animation_time = fmod(time_in_ticks, (f32)animation->mDuration);

	aiMatrix4x4 identity = aiMatrix4x4();
	read_node_hierarchy(animation_time, scene->mRootNode, identity);

	for (uint32_t i = 0; i < bone_transforms.size(); i++)
	{
		bone_transforms[i] = glm::transpose(glm::make_mat4(&m_bone_info[i].final_transformation.a1));
		//debug_bone_transforms[i] = glm::transpose(debug_bone_transforms[i]);
	}

	vulkan_buffer_upload(context, &transform_buffer, bone_transforms.data(), transform_buffer.size);
	vulkan_buffer_upload(context, &debug_transform_buffer, debug_bone_transforms.data(), debug_transform_buffer.size);
}

void skinned_mesh::read_node_hierarchy(float animation_time, const aiNode* node, const aiMatrix4x4& parent_transform)
{
 	std::string node_name(node->mName.data);
	aiMatrix4x4 node_transformation(node->mTransformation);
	const aiNodeAnim* node_anim = find_node_anim(animation, node_name);

	if (node_anim)
	{
		// Get interpolated matrices between current and next frame
		aiMatrix4x4 mat_scale = interpolate_scale(animation_time, node_anim);
		aiMatrix4x4 mat_rotation = interpolate_rotation(animation_time, node_anim);
		aiMatrix4x4 mat_translation = interpolate_translation(animation_time, node_anim);

		node_transformation = mat_translation * mat_rotation * mat_scale;
	}

	aiMatrix4x4 global_transformation = parent_transform * node_transformation;

	if (bone_mapping.find(node_name) != bone_mapping.end())
	{
		uint32_t bone_index = bone_mapping[node_name];
		m_bone_info[bone_index].final_transformation = global_inverse_transform * global_transformation * m_bone_info[bone_index].offset;


		aiVector3D debug_bone_scale;
		aiVector3D debug_bone_rotation;
		aiVector3D debug_bone_translation;
		m_bone_info[bone_index].offset.Decompose(debug_bone_scale, debug_bone_rotation, debug_bone_translation);
		
		debug_bone_scale.x = 1 / debug_bone_scale.x;
		debug_bone_scale.y = 1 / debug_bone_scale.y;
		debug_bone_scale.z = 1 / debug_bone_scale.z;

		//aiVector3D debug_bone_translation(-m_bone_info[bone_index].offset[0][3], -m_bone_info[bone_index].offset[1][3], -m_bone_info[bone_index].offset[2][3]);
		aiMatrix4x4 debug_bone_transform = aiMatrix4x4();
		debug_bone_transform.Translation(-debug_bone_translation, debug_bone_transform);
		debug_bone_transform.Scaling(debug_bone_scale, debug_bone_transform);
		debug_bone_transform.FromEulerAnglesXYZ(-debug_bone_rotation);

		debug_bone_transform = global_inverse_transform * global_transformation * debug_bone_transform;
		debug_bone_transforms[bone_index] = glm::transpose(glm::make_mat4(&debug_bone_transform.a1));
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		read_node_hierarchy(animation_time, node->mChildren[i], global_transformation);
	}
}

const aiNodeAnim* skinned_mesh::find_node_anim(const aiAnimation* animation, const std::string node_name)
{
	for (uint32_t i = 0; i < animation->mNumChannels; i++)
	{
		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		if (std::string(nodeAnim->mNodeName.data) == node_name)
		{
			return nodeAnim;
		}
	}
	return nullptr;
}


void skinned_mesh::destroy()
{
	vulkan_buffer_destroy(context, &transform_buffer);
	vulkan_buffer_destroy(context, &vertex_buffer);
	vulkan_buffer_destroy(context, &index_buffer);

	//debuging buffers
	vulkan_buffer_destroy(context, &debug_vertex_buffer);
	vulkan_buffer_destroy(context, &debug_index_buffer);
	vulkan_buffer_destroy(context, &debug_transform_buffer);
}

vertex_input_description skinned_mesh::get_vertex_input_description()
{
	vertex_input_description result;

	VkVertexInputBindingDescription input_binding_description;
	input_binding_description.binding = 0;
	input_binding_description.stride = sizeof(vertex);
	input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	result.bindings.push_back(input_binding_description);

	std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions(5);

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

	input_attribute_descriptions[3].binding = 0;
	input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	input_attribute_descriptions[3].location = 3;
	input_attribute_descriptions[3].offset = offsetof(vertex, bone_weight);

	input_attribute_descriptions[4].binding = 0;
	input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SINT;
	input_attribute_descriptions[4].location = 4;
	input_attribute_descriptions[4].offset = offsetof(vertex, bone_id);

	result.attributes = input_attribute_descriptions;

	return result;
}

void skinned_mesh::set_animation()
{
	if (selected_anim_index >= animation_count) {
		if (animation_count > 1)
			animation = scene->mAnimations[animation_count - 1];
		else
			animation = nullptr;
	}

	animation = scene->mAnimations[selected_anim_index];
}

