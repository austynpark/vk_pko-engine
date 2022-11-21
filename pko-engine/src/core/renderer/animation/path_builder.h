#ifndef PATH_BUILDER
#define PATH_BUILDER

#include <defines.h>
#include "core/mesh.h"

#include <map>

class vulkan_render_object;

struct path_builder {
	//debug_draw_mesh mesh;
	u32 curve_count;
	std::vector<glm::vec3> control_points;
	std::vector<u32> offset; // store each bezier curve's beginning control point index in control_points
	
	// this is used to simply store stamps for time-velocity
	std::vector<std::pair<f32, f32>> time_velocity_stamps; // key: time, value: velocity

	debug_draw_mesh line_mesh;
	
	f32 total_length = 0.0f;
	f32 frame_sum = 0.0f;
};


/*
* Add full cubic bezier curve (only works if out_path_builder's mesh is empty)
*/
void path_builder_initialize_bezier_curve(
	const glm::vec3& start_point,
	const glm::vec3& knot0,
	const glm::vec3& knot1,
	const glm::vec3& end_point,
	path_builder* out_path_builder
);

/*
* Add another cubic bezier curve which use start_point as the end_point of last bezier curve
* if path_builder has a point, reset and add the curve
*/
void path_builder_add_control_points(
	const glm::vec3& knot0,
	const glm::vec3& knot1,
	const glm::vec3& end_point,
	path_builder* out_path_builder
);

/*
* Insert control points based on Cubic Bezier interpolation
*/
void path_build(path_builder* out_path_builder);

glm::mat4 path_get_along(f32 u, path_builder* path, vulkan_render_object* object);
//glm::mat4 path_get_rotation_matrix(const glm::vec3& p, f32 u, path_builder* path);

glm::mat4 path_line_get_along(f32 dt, const glm::vec3& destination, vulkan_render_object* object);

void path_update(f32 u, path_builder* path);

/*
* curve interpolated in 0 to 1, u must be [0, 1]
*/
glm::vec3 bezier_curve_get_point(
	const glm::vec3& p0,
	const glm::vec3& p1,
	const glm::vec3& p2,
	const glm::vec3& p3,
	f32 dt
);

void path_builder_build_arc_length_table(
	path_builder* out_path_builder
);

// build velocity-time, distance-time table and store in the internal_data struct
// void path_builder_build_time_table(f32 delta_time, path_builder* path_builder);

// time must be [0: 1]
void path_builder_add_time_velocity_stamp(f32 time, f32 velocity, path_builder* out_path_builder);

/*
* Functions for more than 3 key points in time-velocity stamp
*/
// return velocity at certain time[0: 1]
f32 path_builder_get_veloctiy_at_time(f32 time, path_builder* path_builder);

f32 path_builder_get_distance_at_time(f32 vel0, f32 vel1, f32 unit_dt, path_builder* path_builder);

/*
* Functions for EASE-IN AND OUT with distance-time function ( 3 key points in time-velocity stamp)
*/
f32 path_builder_get_sin_velocity_at_time(f32 vel0, f32 t1, f32 t2,f32 time, path_builder* path_builder);
f32 path_builder_get_sin_distance_at_time(f32 vel0, f32 t1, f32 t2, f32 time, path_builder* path_builder);

#endif // !PATH_BUILDER


