#include "path_builder.h"

#include "../vulkan_renderer/vulkan_mesh.h"

#include <iostream>
#include <unordered_map>
#include <array>

#include <algorithm>

struct bezier_curve { // segement of path
	std::array<glm::vec3, 4> control_points; // cubic bezier curve
	//std::map<f32, f32> arc_table; // curve's arc table 
	f32 length = 0.0f; // curve length
	f32 begin_length = 0.0f;
};

struct path_data {
	std::vector<bezier_curve> curves;

	f32 delta_time = 0.01666f; // unit of time for each iteration (time_velocity_table[i]'s time = i * delta_time)
	//std::vector<f32> time_velocity_table;
	//std::vector<f32> time_distance_table;
	//std::vector<std::pair<f32, f32>> concatenated_arc_table; // final arc table (key: length, value: u)
};

//struct 
struct path_builder_internal_data {
	std::vector<path_data> path_table;
	// for each path
		// for each curve
			// end of u, length
};

static path_builder_internal_data internal_data;
static std::unordered_map<path_builder*, u32> path_key_table;

std::pair<f32, f32> binary_search_arc_table(const f32 length, u32 start, u32 end, const std::vector<std::pair<f32, f32>>& arc_table);
f32 get_path_distance(f32 u, path_builder* path);

//static delta_time = 0.01666f;
float bezier_curve_build_arc_length_table(const std::array<glm::vec3, 4>& control_points, f32 start, f32 end) {

	if (start >= end)
		return 0.0f;

	f32 distance = 0;
	
	glm::vec3 p0 = bezier_curve_get_point(control_points[0], control_points[1], control_points[2], control_points[3], start);
	glm::vec3 p1 = bezier_curve_get_point(control_points[0], control_points[1], control_points[2], control_points[3], end);

	glm::vec3 mid_point = bezier_curve_get_point(control_points[0], control_points[1], control_points[2], control_points[3], (start + end) * 0.5f);
	distance = glm::distance(p0, p1);

	f32 mid_distance = glm::distance(p0, mid_point) + glm::distance(mid_point, p1);

	if ((mid_distance - distance) > 0.001f)
	{
		f32 half0 = bezier_curve_build_arc_length_table(control_points, start, (start+end) * 0.5f);
		f32 half1 = bezier_curve_build_arc_length_table(control_points, (start+end) * 0.5f, end);

		return half0 + half1;
	}

	//out_arc_table[end] =  out_arc_table[start] + distance;
	return distance;
}

void path_builder_initialize(path_builder* out_path_builder)
{
	if (path_key_table.find(out_path_builder) == path_key_table.end()) {

		path_data data{};

		path_key_table[out_path_builder] = internal_data.path_table.size();
		internal_data.path_table.push_back(data);
	}

	out_path_builder->control_points.clear();
	//out_path_builder->end_idx = -1;
	out_path_builder->curve_count = 0;
}

void path_builder_initialize_bezier_curve(const glm::vec3& start_point, const glm::vec3& knot0, const glm::vec3& knot1, const glm::vec3& end_point, path_builder* out_path_builder)
{
	path_builder_initialize(out_path_builder);

	out_path_builder->control_points = {
		start_point,
		knot0,
		knot1,
		end_point
	};

	++out_path_builder->curve_count;
}

void path_builder_add_control_points(const glm::vec3& knot0, const glm::vec3& knot1, const glm::vec3& end_point, path_builder* out_path_builder)
{
	if (out_path_builder->curve_count == 0) {
		return;
	}

	out_path_builder->control_points.insert(out_path_builder->control_points.end(),
		{
			knot0,
			knot1,
			end_point
		}
	);

	++out_path_builder->curve_count;
}

// make vertices for path rendering & insert extra control points between existing control points-
// to prevent sharp edge and make smooth curve while linking end control point and start
void path_build(path_builder* out_path_builder)
{
	u32 curve_count = out_path_builder->curve_count;

	if (curve_count == 0) return;
	
	const u32 curve_interval = 3;
	u32 index = 1; // out_path_builder->mesh.points[index] = Pi

	std::vector<glm::vec3>& control_points = out_path_builder->control_points;

	//glm::vec3 Pi = points[index];
	glm::vec3 a(0); // a(i)
	glm::vec3 b(0); // b(i+1)


	u32 control_point_size = out_path_builder->control_points.size();
	
	std::vector<glm::vec3> out_control_points;

	out_control_points.push_back(out_path_builder->control_points.front());

	out_path_builder->curve_count = 0;
	
	if (path_key_table.find(out_path_builder) == path_key_table.end())
		return;

	u32 path_id = path_key_table[out_path_builder];
	std::vector<bezier_curve>& curves = internal_data.path_table[path_id].curves;
	const f32 unit_dt = internal_data.path_table[path_id].delta_time;

	bezier_curve curve{};

	// build new control points (smoothing)
	for (u32 i = 0; i < control_point_size; ++i) {

		glm::vec3 Pi_sub1 = (i == 0) ? control_points.back() : control_points[i - 1];
		glm::vec3 Pi1 = control_points[(i + 1) % control_point_size];
		glm::vec3 Pi2 = control_points[(i + 2) % control_point_size];
	
		a = control_points[i] + (Pi1 - Pi_sub1) / 8.0f;
		b = Pi1 - (Pi2 - control_points[i]) / 8.0f;

		curve.control_points[0] = out_control_points.back();
		curve.control_points[1] = a;
		curve.control_points[2] = b;

		out_control_points.push_back(a);
		out_control_points.push_back(b);
		out_control_points.push_back(control_points[(i + 1) % control_point_size]);

		curve.control_points[3] = out_control_points.back();

		//out_curve_table[path_id][i-1][0] = out_control_points.back();
		curves.push_back(curve);
	}

	out_path_builder->control_points = out_control_points;

	std::vector<glm::vec3>& out_points = out_path_builder->line_mesh.points;
/*
* [Pi, ai, bi+1, Pi+1]
* qi = pi + (ai - pi) / dt[0:1]
* qi+1 = ai + (bi+1 - ai) / dt
* qi+2 = bi+1 + (pi+1 - bi+1) / dt
* ri = qi + (qi+1 - qi) / dt
* ri+1 = qi+1 + (qi+2 - qi+1) / dt
* si = ri + (ri+1 - ri) / dt
* si is point on the curve at 'dt' between [pi, pi+1]
*/
	out_path_builder->curve_count = curves.size();

	//out_path_builder->time_velocity_stamps.emplace_back(1.0f, 0.0f);

	path_builder_build_arc_length_table(out_path_builder);

	//path_builder_build_time_table(unit_dt, out_path_builder);

	
	//out_points.push_back(out_control_points[0]);
	// insert points to mesh to draw the path
	for (u32 i = 0; i < out_path_builder->curve_count; ++i) {
		u32 control_index = (curve_interval * i);
		
		glm::vec3 pi = out_control_points[control_index++];
		glm::vec3 ai = out_control_points[control_index++];
		glm::vec3 bi1 = out_control_points[control_index++];
		glm::vec3 pi1 = out_control_points[control_index];


		for (u32 l = 0; l <= 60; ++l) {
			f32 dt = unit_dt * l;
		//glm::vec3 qi = pi + (ai - pi) * dt;
		//glm::vec3 qi1 = ai + (bi1 - ai) * dt;
		//glm::vec3 qi2 = bi1 + (pi1 - bi1) * dt;
		//glm::vec3 ri = qi + (qi1 - qi) * dt;
		//glm::vec3 ri1 = qi1 + (qi2 - qi1) * dt;
		//glm::vec3 si = ri + (ri1 - ri) * dt;

			out_points.push_back(
				bezier_curve_get_point(
					pi,
					ai,
					bi1,
					pi1,
					dt
				)
			);
		}

		//out_points.push_back(pi1);
		out_path_builder->offset.push_back(out_points.size() - 1);
	}
}

glm::mat4 path_get_along(f32 dt, path_builder* path, vulkan_render_object* object)
{
	f32 frame_sum = path->frame_sum;
	frame_sum += (dt * 0.1f);
	i32 int_frame = frame_sum;
	frame_sum -= int_frame;

	u32 key = path_key_table[path];
	const path_data& data = internal_data.path_table[key];

	// get S from distance-time table
	f32 s = 0.0f;
	{
		s = get_path_distance(frame_sum, path);
	}

	f32 current_path_length = 0.0f;
	f32 next_path_length = 0.0f;
	u32 selected_curve_idx = 0;
	f32 selected_u = 0.0f;
	// find u matches s

	for (u32 i = 0; i < path->curve_count; ++i) {
		if (data.curves[i].length + data.curves[i].begin_length >= s) {
			selected_curve_idx = i;
			next_path_length = data.curves[i].length + data.curves[i].begin_length;
			current_path_length = data.curves[i].begin_length;
			break;
		}
	}

	selected_u = (s - current_path_length) / (next_path_length - current_path_length);

	glm::vec3 result = bezier_curve_get_point(
		data.curves[selected_curve_idx].control_points[0],
		data.curves[selected_curve_idx].control_points[1],
		data.curves[selected_curve_idx].control_points[2],
		data.curves[selected_curve_idx].control_points[3],
		selected_u
	);

	// object orientation along the path
	glm::vec3 pi1 = bezier_curve_get_point(
		data.curves[selected_curve_idx].control_points[0],
		data.curves[selected_curve_idx].control_points[1],
		data.curves[selected_curve_idx].control_points[2],
		data.curves[selected_curve_idx].control_points[3],
		selected_u + dt
	);

	glm::vec3 pi2 = bezier_curve_get_point(
		data.curves[selected_curve_idx].control_points[0],
		data.curves[selected_curve_idx].control_points[1],
		data.curves[selected_curve_idx].control_points[2],
		data.curves[selected_curve_idx].control_points[3],
		selected_u + dt * 2
	);

	glm::vec3 pi3 = bezier_curve_get_point(
		data.curves[selected_curve_idx].control_points[0],
		data.curves[selected_curve_idx].control_points[1],
		data.curves[selected_curve_idx].control_points[2],
		data.curves[selected_curve_idx].control_points[3],
		selected_u + dt * 3
	);

	glm::vec3 coi = (pi1 + pi2 + pi3) / 3.0f;

	glm::vec3 roll = glm::normalize(coi - result);
	glm::vec3 pitch = glm::cross(glm::vec3(0, 1, 0), roll);
	glm::vec3 yaw = glm::normalize(glm::cross(roll, pitch));
	
	glm::mat4 euler_mat(1.0f);
	euler_mat[0] = glm::vec4(pitch, 0);
	euler_mat[1] = glm::vec4(yaw, 0);
	euler_mat[2] = glm::vec4(roll, 0);

	/*
	std::cout << "x: " << result.x << " ";
	std::cout << "y: " << result.y << " ";
	std::cout << "z: " << result.z << std::endl;
	*/

	object->position = result;

	return object->get_transform_matrix() * euler_mat;
}

void path_update(f32 dt, path_builder* path)
{
	path->frame_sum += (dt * 0.1f);
	i32 int_frame = path->frame_sum;
	path->frame_sum -= int_frame;
}

glm::vec3 bezier_curve_get_point(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, f32 u)
{
	f32 cubic_u = u * u * u;
	f32 square_u = u * u;

	glm::vec3 point =
		(-cubic_u + 3 * square_u - 3 * u + 1) * p0 +
		(3 * cubic_u - 6 * square_u + 3 * u) * p1 +
		(-3 * cubic_u + 3 * square_u) * p2 +
		cubic_u * p3;

	return point;
}

void path_builder_build_arc_length_table(path_builder* out_path_builder)
{
	if (path_key_table.find(out_path_builder) != path_key_table.end()) {
		u32 key = path_key_table[out_path_builder];
		std::vector<bezier_curve>& curves = internal_data.path_table[key].curves;

		const u32 curve_interval = 3;
		const u32 delta_time_value = 0.01666f;
		f32 dt = delta_time_value;
		f32 result_distance = 0;
		u32 last_curve_end_index = 0;
		for (u32 i = 0; i < out_path_builder->curve_count; ++i) {
			
			f32 dist = bezier_curve_build_arc_length_table(curves[i].control_points, 0.0f, 1.0f);
			
			//arc_table
			curves[i].begin_length = result_distance;
			result_distance += dist;
			curves[i].length = dist;
		}

		out_path_builder->total_length = result_distance;

		// rebuild the arc_length_table and normalize it into [0, 1] to match with p(u)
		//std::vector<std::pair<f32, f32>>& rebuilt_arc_table = internal_data.path_table[key].concatenated_arc_table;

		result_distance = 0.0f;

		for (u32 i = 0; i < out_path_builder->curve_count; ++i) {
			f32 prev_dist = result_distance;
			//rebuilt_arc_table.emplace_back(result_distance / out_path_builder->total_length, static_cast<f32>(i) / out_path_builder->curve_count);
			result_distance += curves[i].length;
			curves[i].length /= out_path_builder->total_length;
			curves[i].begin_length /= out_path_builder->total_length;
		}
	}
}

/*
void path_builder_build_time_table(f32 delta_time, path_builder* path_builder)
{
	if (path_key_table.find(path_builder) != path_key_table.end()) {
		u32 key = path_key_table[path_builder];
		std::vector<f32>& vel_table = internal_data.path_table[key].time_velocity_table;
		std::vector<f32>& dist_table = internal_data.path_table[key].time_distance_table;

		u32 key_point_count = path_builder->time_velocity_stamps.size();
		b8 use_ease_inout = (key_point_count == 3); // more than 3 key points (user_defined velocity-time functions)
		delta_time *= 0.1f;
		u32 dt_count = (1.0 / delta_time);

		if (!use_ease_inout) {
			f32 v0 = path_builder_get_veloctiy_at_time(0, path_builder);
			vel_table.push_back(v0);
			dist_table.push_back(0);

			f32 total_distance = 0.0f;
			f32 segment_distance = 0.0f;

			for (u32 i = 1; i <= dt_count; ++i) {

				f32 v1 = path_builder_get_veloctiy_at_time(i * delta_time, path_builder);
				vel_table.push_back(v1);

				segment_distance = path_builder_get_distance_at_time(v0, v1, delta_time, path_builder);
				total_distance += segment_distance;

				dist_table.push_back(total_distance);
				v0 = v1;
			}

			for (auto& vel : vel_table) {
				vel /= total_distance;
			}

			for (auto& dist : dist_table) {
				dist /= total_distance;
			}
		}
		else {

			f32 v1 = path_builder->time_velocity_stamps[0].second;
			f32 t1 = path_builder->time_velocity_stamps[0].first;
			f32 t2 = path_builder->time_velocity_stamps[1].first;

			for (u32 i = 0; i <= dt_count; ++i) {
				vel_table.push_back(path_builder_get_sin_velocity_at_time(v1, t1, t2, i * delta_time, path_builder));
				dist_table.push_back( path_builder_get_sin_distance_at_time(v1, t1, t2, i * delta_time, path_builder));
			}

			f32 total_distance = dist_table.back();

			for (auto& vel : vel_table) {
				vel /= total_distance;
			}

			for (auto& dist : dist_table) {
				dist /= total_distance;
			}

		}
	}
}
*/

// get arc length with u [0:1]
f32 get_path_distance(f32 u, path_builder* path)
{
	if (path_key_table.find(path) != path_key_table.end()) {
		u32 key = path_key_table[path];

		f32 v1 = path->time_velocity_stamps[0].second;
		f32 t1 = path->time_velocity_stamps[0].first;
		f32 t2 = path->time_velocity_stamps[1].first;

		f32 distance = path_builder_get_sin_distance_at_time(v1, t1, t2, u, path);
		f32 total_distance = path_builder_get_sin_distance_at_time(v1, t1, t2, 1.0f, path);

		return distance / total_distance;
	}

	return 0.0f;
}

void path_builder_add_time_velocity_stamp(f32 time, f32 velocity, path_builder* out_path_builder)
{
	if (time < 0.0f || time > 1.0f)
		return;

	out_path_builder->time_velocity_stamps.emplace_back(time, velocity);
}

f32 path_builder_get_veloctiy_at_time(f32 time, path_builder* path_builder)
{
	if (time <= 0.0f || time >= 1.0f)
		return 0.0f;

	const std::vector<std::pair<f32, f32>>& stamps = path_builder->time_velocity_stamps;

	f32 last_time = 0.0f;
	f32 last_vel = 0.0f;

	for (const auto& time_velocity : stamps) {

		f32 current_time = time_velocity.first;
		f32 current_vel = time_velocity.second;

		if (time_velocity.first >= time) {
			return (last_vel)+((current_vel - last_vel) * (time - last_time)) / (current_time - last_time);
		}

		last_time = time_velocity.first;
		last_vel = time_velocity.second;
	}

	return 0.0f;
}

// time_idx = end_time_idx
// this function is for multiple keys not only for 3 keys (more than 3 velocity-time stamps)
f32 path_builder_get_distance_at_time(f32 vel0, f32 vel1, f32 unit_dt, path_builder* path_builder)
{
	f32 area = 0;

	if (vel0 < vel1) {

		// triangle area = (width) * (height) * 0.5f
		area = (unit_dt) * (vel1 - vel0) * 0.5f;

		if (vel0 != 0.0f) { // triangle + square
			area += unit_dt * vel0;
		}
	}
	else if (vel0 > vel1) { // 
		
		area = (unit_dt) * (vel0 - vel1) * 0.5f;

		if (vel1 != 0.0f) {
			area += unit_dt * vel1;
		}

	}
	else { // square
		area = (unit_dt) * (vel0);
	}

	return area;
}

// only three velocity-time stamps with constant velocity between two stamps
f32 path_builder_get_sin_velocity_at_time(f32 vel0, f32 t1, f32 t2, f32 time, path_builder* path_builder)
{
	f32 velocity = 0.0f;

	if (time >= 0 && time <= t1) {
		velocity = (vel0 * time) / t1;
	}
	else if (time > t1 && time < t2) {
		velocity = vel0;
	}
	else if(time >= t2 && time <= 1.0f) {
		velocity = vel0 * (1.0f - time) / (1.0f - t2);
	}

	return velocity;
}

// only three velocity-time stamps with constant velocity between two stamps
f32 path_builder_get_sin_distance_at_time(f32 vel0, f32 t1, f32 t2, f32 time, path_builder* path_builder)
{
	f32 distance = 0.0f;

	if (time >= 0 && time <= t1) {
		distance = (vel0 * time * time * 0.5f) / t1;
	}
	else if (time > t1 && time < t2) {
		distance = vel0 * (time - t1 * 0.5f);
	}
	else if(time >= t2 && time <= 1.0f) {
		distance = (vel0 * (time - t2) * 0.5f) * (2.0f - time - t2) / (1 - t2) + vel0 * (t2 - t1 * 0.5f);
	}

	return distance;
}

std::pair<f32, f32> binary_search_arc_table(const f32 length, u32 start, u32 end, const std::vector<std::pair<f32, f32>>& arc_table)
{
	std::pair<f32, f32> result{0, 0};

	while (start < end) {
		u32 mid = (start + end) * 0.5f;
		f32 mid_length = arc_table[mid].first;

		if (end - start == 1) {
			return arc_table[mid];
		}

		if (length < mid_length) {

			end = mid;
		}
		else if (length > mid_length) {

			start = mid;
		}
		else { // exactly match (but probably not)
			return arc_table[mid];
		}
	}

	std::cout << "binary search empty return" << std::endl;
	return result;
}
