#ifndef MATH_TYPES_H
#define MATH_TYPES_H

#include "defines.h"

#include <immintrin.h>
#include <cassert>
#include <cmath>

#include <glm/glm.hpp>
//#include <fvec.h>

namespace pko_math {

	typedef struct vec3_u {

		inline vec3_u() {
			elements[0] = 0.0f;
			elements[1] = 0.0f;
			elements[2] = 0.0f;
		}

		inline vec3_u(f32 v) {
			elements[0] = v;
			elements[1] = v;
			elements[2] = v;
		}

		inline vec3_u(f32 x, f32 y, f32 z) {
			elements[0] = x;
			elements[1] = y;
			elements[2] = z;
		}

		inline vec3_u(const glm::vec3 v) {
			memcpy_s(this, sizeof(vec3_u), &v, sizeof(glm::vec3));
		}

		inline f32& operator[](u32 index) {
			assert(index < 3);
			return elements[index];
		}

		inline f32 length() const {
			return (
				elements[0] * elements[0] +
				elements[1] * elements[1] +
				elements[2] * elements[2]
				);
		}

		inline vec3_u normalize() {
			f32 len = length();

			if (abs(len - 1.0f) > 0.001f) {
				f32 len_inv = 1 / len;

				elements[0] *= len_inv;
				elements[1] *= len_inv;
				elements[2] *= len_inv;
			}

			return (*this);
		}

		union {
			f32 elements[3];
			struct {
				union {
					f32 x,
						r,
						s;
				};

				union {
					f32 y,
						g,
						t;
				};

				union {
					f32 z,
						b,
						p;
				};
			};
		};
	} vec3;

	typedef struct vec4_u {

		inline vec4_u() {
			data = _mm_setzero_ps();
		}

		inline vec4_u(f32 v) {
			data = _mm_set_ps1(v);
		}

		inline vec4_u(f32 x, f32 y, f32 z, f32 w) {
			data = _mm_setr_ps(x, y, z, w);
		}

		inline vec4_u(const glm::vec3 v, f32 w) {
			elements[0] = v.x;
			elements[1] = v.y;
			elements[2] = v.z;
			elements[3] = w;
		}

		inline vec4_u(const glm::vec4 v) {
			memcpy_s(this, sizeof(vec4_u), &v, sizeof(glm::vec4));
		}

		inline vec4_u(vec3 v, f32 w) {
			elements[0] = v.x;
			elements[1] = v.y;
			elements[2] = v.z;
			elements[3] = w;
		}

		inline f32& operator[](u32 index) {
			assert(index < 4);

			return elements[index];
		}

		inline vec4_u operator-() const {
			vec4 result;
			result.data = _mm_sub_ps(_mm_setzero_ps(), data);
			return result;
		}

		inline f32 length() const {
			return _mm_cvtss_f32(_mm_sqrt_ps(data));
		}

		inline vec4_u normalize() {
			f32 len = length();

			if (abs(len - 1.0f) > 0.001f) {
				f32 len_inv = 1 / len;

				__m128 simd_s = _mm_broadcast_ss((const f32*)&len_inv);
				data = _mm_mul_ps(data, simd_s);
			}

			return (*this);
		}

		union {
			f32 elements[4];
			__m128 data;
			struct {
				union {
					f32 x,
						r,
						s;
				};

				union {
					f32 y,
						g,
						t;
				};

				union {
					f32 z,
						b,
						p;
				};

				union {
					f32 w,
						a,
						q;
				};
			};
		};
	} vec4;

	typedef vec4 quat;
}



#endif // !MATH_TYPES_H
