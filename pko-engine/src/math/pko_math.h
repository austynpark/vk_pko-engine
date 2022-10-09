#ifndef PKO_MATH_H
#define PKO_MATH_H

#include "math_types.h"

#include <memory>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>


namespace  pko_math {

	inline vec3 vec3_create() {
		vec3 result;
		memset(&result, 0.0f, sizeof(result));
		return result;
	}

	inline vec3 operator+(const vec3& v1, const vec3& v2) {
		return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
	}

	inline vec3 operator-(const vec3& v1, const vec3& v2) {
		return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
	}

	inline vec3 operator/(const vec3& v, f32 s) {
		return vec3(v.x / s, v.y / s, v.z / s);
	}

	inline vec3 operator*(const vec3& v, f32 s) {
		return vec3(v.x * s, v.y * s, v.z * s);
	}

	inline vec3 operator/(f32 s, const vec3& v) {
		return vec3(v.x / s, v.y / s, v.z / s);
	}

	inline vec3 operator*(f32 s, const vec3& v) {
		return vec3(v.x * s, v.y * s, v.z * s);
	}

	inline f32 dot_product(const vec3& v1, const vec3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	inline vec3 cross_product(const vec3& v1, const vec3& v2) {

		return vec3(
			v1.y * v2.z - v1.z * v2.y,
			-(v1.x * v2.z - v1.z * v2.x),
			v1.x * v2.y - v1.y * v2.x
		);
	}

	inline vec4 vec4_create() {
		vec4 result;
		result.data = _mm_setzero_ps();
		return result;
	}

	inline vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
		vec4 result;
		result.data = _mm_setr_ps(x, y, z, w);
		return result;
	}

	inline  vec4 operator+(const vec4& v1, const vec4& v2) {
		vec4 result;
		result.data = _mm_add_ps(v1.data, v2.data);
		return result;
	}

	inline vec4 operator-(const vec4& v1, const vec4& v2) {
		vec4 result;
		result.data = _mm_sub_ps(v1.data, v2.data);
		return result;
	}

	inline vec4 operator/(const vec4& v1, f32 s) {
		vec4 result;
		__m128 scalar = _mm_set_ps1(s);
		result.data = _mm_div_ps(v1.data, scalar);
		return v1;
	}

	inline vec4 vec4_mul(const vec4& v1, const vec4& v2) {
		vec4 result;
		result.data = _mm_mul_ps(v1.data, v2.data);

		return result;
	}

	inline vec4 vec4_mul_scale(const vec4& v1, f32 s) {
		__m128 simd_s = _mm_broadcast_ss((const f32*)&s);
		vec4 result;
		result.data = (_mm_mul_ps(v1.data, simd_s));
		return result;
	}

	inline f32 dot_product(const vec4& v1, const vec4& v2) {
		__m128 xmm = _mm_mul_ps(v1.data, v2.data);
		xmm = _mm_hadd_ps(xmm, xmm);
		xmm = _mm_hadd_ps(xmm, xmm);

		return _mm_cvtss_f32(xmm);
	}

	inline f32 dot_product(const quat& q1, const quat& q2) {
		__m128 xmm = _mm_mul_ps(q1.data, q2.data);
		xmm = _mm_hadd_ps(xmm, xmm);
		xmm = _mm_hadd_ps(xmm, xmm);

		return _mm_cvtss_f32(xmm);
	}

	// identity quaternion
	inline quat quat_create() {
		return quat(1.0f, 0, 0, 0);
	}

	inline quat quat_create(const vec3& v, f32 s) {
		return quat(s, v.x, v.y, v.z);
	}

	inline quat quat_conjugate(const quat& q) {
		return quat(q.w, -q.x, -q.y, -q.z);
	}

	inline quat operator+(const quat& v1, const quat& v2) {
		quat result;
		result.data = _mm_add_ps(v1.data, v2.data);
		return result;
	}

	inline quat operator/(const quat& v1, f32 s) {
		quat result;
		__m128 scalar = _mm_set_ps1(s);
		result.data = _mm_div_ps(v1.data, scalar);
		return result;
	}

	inline quat quat_mul(const quat& q1, const quat& q2) {
		quat out_quaternion = quat_create();

		out_quaternion.x = q1.x * q2.w +
			q1.y * q2.z -
			q1.z * q2.y +
			q1.w * q2.x;

		out_quaternion.y = -q1.x * q2.z +
			q1.y * q2.w +
			q1.z * q2.x +
			q1.w * q2.y;

		out_quaternion.z = q1.x * q2.y -
			q1.y * q2.x +
			q1.z * q2.w +
			q1.w * q2.z;

		out_quaternion.w = -q1.x * q2.x -
			q1.y * q2.y -
			q1.z * q2.z +
			q1.w * q2.w;

		return out_quaternion;
	}

	inline vec3 quat_mul_vec3(const quat& q, const vec3& r) {
		
		float s = q.w;
		vec3 v(q.x, q.y, q.z);
		return 2.0f * ((s * s - 0.5f) * r + dot_product(v, r) * v + s * cross_product(v, r));
		
		/*
		vec3 quat_v = vec3(q.x, q.y, q.z);
		vec3 uv = cross_product(quat_v, v);
		vec3 uuv = cross_product(quat_v, uv);

		return v + ((uv * q.w) + uuv) * 2.0f;
		*/
	}

	inline vec4 quat_mul_vec4(const quat& q, const vec4& v) {
		return vec4(quat_mul_vec3(q, vec3(v.x, v.y, v.z)), v.w);
	}

	inline quat quat_mul_scale(const quat& q1, f32 s) {
		__m128 simd_s = _mm_broadcast_ss((const f32*)&s);
		quat result;
		result.data = (_mm_mul_ps(q1.data, simd_s));
		return result;
	}

	inline quat quat_inverse(const quat& q) {
		__m128 xmm = _mm_mul_ps(q.data, q.data);
		xmm = _mm_hadd_ps(xmm, xmm);
		xmm = _mm_hadd_ps(xmm, xmm);

		f32 inv = 1 / (_mm_cvtss_f32(xmm));

		quat result = quat_mul_scale(quat_conjugate(q), inv);
		return result;
	}

	inline glm::mat4 quat_to_matrix(const quat& q) {
		glm::mat4 out_matrix(1.0f);

		quat n = q;
		n.normalize();

		out_matrix[0][0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
		out_matrix[0][1] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
		out_matrix[0][2] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;

		out_matrix[1][0] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
		out_matrix[1][1] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
		out_matrix[1][2] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;

		out_matrix[2][0] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;
		out_matrix[2][1] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;
		out_matrix[2][2] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;
		
		return out_matrix;
	}

	inline vec3 rotate(const quat& q, const vec3& v) {
		return quat_mul_vec3(q, v);
	}

	inline vec4 rotate(const quat& q, const vec4& v) {
		return quat_mul_vec4(q, v);
	}

	inline quat rotate(const quat& q, const f32 angle, const vec3& v) {
		vec3 normalized_v = v;
		normalized_v.normalize();

		f32 angle_rad = glm::radians(angle);
		f32 sin_val = sin(angle_rad * 0.5f);

		return quat_mul(q,
			quat(
				cos(angle_rad * 0.5f),
				normalized_v.x * sin_val,
				normalized_v.y * sin_val,
				normalized_v.z * sin_val
			)
		);
	}

// Interpolation functions

	inline quat lerp(const quat& x, const quat& y, f32 a) {
		
		assert(a >= 0.0f);
		assert(a <= 1.0f);

		return quat_mul_scale(x, (1.0f - a)) + quat_mul_scale(y, a);
	}

	inline quat slerp(const quat& x, const quat& y, f32 t) {
		
		quat z = y;
		f32 cos_theta = dot_product(x, y);

		if (cos_theta < 0.0f) {
			z = -y;
			cos_theta = -cos_theta;
		}

		// Perform a linear interpolation when cos_theta is close to 1 to avoid side effect of sin(angle) becoing a zero denominator
		if (cos_theta > 1.0f - std::numeric_limits<float>().epsilon()) {
			return quat(
				glm::mix(x.w, z.w, t),
				glm::mix(x.x, z.x, t),
				glm::mix(x.y, z.y, t),
				glm::mix(x.z, z.z, t)
			);
		}

		f32 angle = acos(cos_theta);

		pko_math::quat q1 = quat_mul_scale(x, sin((1.0f - t) * angle));
		pko_math::quat q2 = quat_mul_scale(z, sin(t * angle));
		pko_math::quat q3 = q1 + q2;

		pko_math::quat result = q3 / sin(angle);

		return result;
	}

	inline vec3 lerp(const vec3& x, const vec3& y, f32 a) {

		assert(a >= 0.0f);
		assert(a <= 1.0f);

		return x + a * (y - x);
	}

	inline f32 elerp(f32 s0, f32 s1, f32 t) {
		return s0 * pow(s1 / s0, t);
	}

	inline vec3 to_vec3(const aiVector3D& v) {
		vec3 result;
		memcpy_s(&result, sizeof(vec3), &v, sizeof(vec3));
		return result;
	}

	inline vec3 to_vec3(const glm::vec3& v) {
		vec3 result;
		memcpy_s(&result, sizeof(vec3), &v, sizeof(vec3));
		return result;
	}

	inline quat to_quat(const aiQuaternion& q) {
		return quat(q.w, q.x, q.y, q.z);
	}

	inline quat to_quat(const glm::quat& q) {
		return quat(q.w, q.x, q.y, q.z);
	}

	inline glm::mat4 assimp_to_glm(const aiMatrix4x4& mat) {

		return glm::transpose(glm::make_mat4(&mat.a1));
	}

}


#endif // !PKO_MATH_H
