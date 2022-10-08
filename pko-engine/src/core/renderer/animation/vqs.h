#ifndef VQS_H
#define VQS_H

#include "defines.h"
#include "math/math_types.h"

#include <glm/glm.hpp>

class VQS {
public:
	VQS();

	// v = translate, q = rotation, s = scale
	VQS(pko_math::vec3 translate, pko_math::quat rotation, f32 scale);
	VQS(const VQS& vqs0, const VQS& vqs1);

	// transform
	pko_math::vec3 operator*(const pko_math::vec3& r) const;
	VQS	operator*(const VQS& vqs) const;
	glm::mat4 to_matrix() const;

	pko_math::vec3 v;
	pko_math::quat q;
	f32 s;
private:

};

/*
* For translation, use Lerp
* rotation		 , use Slerp
* scale			 , use Elerp
*/
VQS incremental_interpolate_vqs(const VQS& vqs0, const VQS& vqs1);
VQS interpolate_vqs(const VQS& vqs0, const VQS& vqs1, f32 t);

#endif // !VQS_H
