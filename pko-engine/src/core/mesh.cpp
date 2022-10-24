#include "mesh.h"

void debug_mesh_add_line(const glm::vec3& point0, const glm::vec3& point1, debug_draw_mesh* mesh)
{
	mesh->points.push_back(point0);
	mesh->points.push_back(point1);
}

void debug_mesh_add_line(const glm::vec3& point1, debug_draw_mesh* mesh)
{
	if (!mesh->points.empty()) {
		mesh->points.push_back(mesh->points.back());
		mesh->points.push_back(point1);
	}
}

void debug_mesh_link_begin_end(debug_draw_mesh* mesh)
{
	if (!mesh->points.empty()) {
		mesh->points.push_back(mesh->points.back());
		mesh->points.push_back(mesh->points.front());
	}
}
