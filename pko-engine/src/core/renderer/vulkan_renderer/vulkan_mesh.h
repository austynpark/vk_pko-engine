#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_image.h"
#include <glm/glm.hpp>

struct RenderContext;
struct cgltf_data;

typedef struct ModelConstants
{
	glm::mat4 model;
	glm::mat3 normal_matrix;
	glm::vec3 padding;
} ModelConstants;

typedef struct RawImageData
{
	void* pixels;
	u32 width;
	u32 height;
	u32 channels;
	u32 size;
} RawImageData;

typedef struct MeshRange
{
	u32 vertex_offset;
	u32 vertex_count;
	u32 index_offset;
	u32 index_count;
	u32 material_index;
} MeshRange;

typedef struct MaterialInfo
{
	u32 base_color_texture_index;
	u32 normal_texture_index;
	u32 metallic_roughness_texture_index;
	u32 occlusion_texture_index;
} MaterialInfo;

typedef struct MeshInstance
{
	glm::mat4 model_matrix;
	u32 range_index;
} MeshInstance;

typedef struct Mesh
{
	f32* positions;
	f32* normals;
	f32* uvs;
	u32* indices;
	MeshRange* ranges;
	MeshInstance* instances;
	MaterialInfo* materials;
	u32 material_count;
} Mesh;

b8 create_deinterleaved_mesh_buffers(RenderContext* context, const Mesh* mesh,
	Buffer* out_positions, Buffer* out_normals, Buffer* out_uvs,
	Buffer* out_indices, u32* out_vertex_count,
	u32* out_index_count);

// GLTF Loaded to single Mesh
b8 load_gltf_from_file(RenderContext* context, const char* filename, Mesh** out_meshes,
	Texture** out_textures);

void free_mesh(Mesh* mesh, u32 mesh_count);

#endif  // !VULKAN_MESH_H
