#ifndef PATH_BUILDER
#define PATH_BUILDER

#include <defines.h>

#include <core/mesh.h>

// CUBIC BEZIER CURVE

void add_control_points(
	const glm::vec3& point0,
	const glm::vec3& point1,
	const glm::vec3& point2,
	debug_draw_mesh* mesh
);


#endif // !PATH_BUILDER


