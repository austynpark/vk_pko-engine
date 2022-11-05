#pragma once

#define ANIMATION_ON 1

#include <defines.h>
#include <glm/glm.hpp>

#include <vector>

constexpr u32 MAX_BONES = 64;
constexpr u32 MAX_BONES_PER_VERTEX = 4;

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
};

struct debug_draw_mesh {
	std::vector<glm::vec3> points;
	std::vector<u32> indices;
};

void debug_mesh_add_line(const glm::vec3& point0, const glm::vec3& point1, debug_draw_mesh* mesh);

// link with existing point, if mesh is empty, just return;
void debug_mesh_add_line(const glm::vec3& point1, debug_draw_mesh* mesh);

// link first and last node
void debug_mesh_link_begin_end(debug_draw_mesh* mesh);
