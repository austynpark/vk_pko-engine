#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_types.inl"

#include <vec3.hpp>

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
};

struct mesh {
	std::vector<vertex> vertices;
	vulkan_allocated_buffer vertex_buffer;
};

#endif // !VULKAN_MESH_H
