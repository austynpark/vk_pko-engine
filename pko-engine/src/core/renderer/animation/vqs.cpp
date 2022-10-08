#include "vqs.h"

#include "math/pko_math.h"

using namespace pko_math;

VQS::VQS() : v(), q(pko_math::quat_create()), s(1.0f)
{
}

VQS::VQS(pko_math::vec3 translate, pko_math::quat rotation, f32 scale) : v(translate), q(rotation), s(scale)
{
}

VQS::VQS(const VQS& vqs0, const VQS& vqs1)
{
	v = vqs0 * vqs1.v;
	q = quat_mul(vqs0.q, vqs1.q);
	s = vqs0.s * vqs1.s;
}

pko_math::vec3 VQS::operator*(const pko_math::vec3& r) const
{
	return rotate(q, s * r) + v;
}

VQS VQS::operator*(const VQS& vqs) const
{
	VQS result(*this, vqs);
	return result;
}

glm::mat4 VQS::to_matrix() const
{
	glm::mat4 result; // transform_matrix
	
	result = quat_to_matrix(q) * glm::scale(result, glm::vec3(s));
	result = glm::translate(result, glm::vec3(v.x, v.y, v.z));

	return result;
}

VQS interpolate_vqs(const VQS& vqs0, const VQS& vqs1, f32 t)
{
	VQS vqs;

	vqs.v = lerp(vqs0.v, vqs1.v, t);
	vqs.q = slerp(vqs0.q, vqs1.q, t);
	vqs.s = elerp(vqs0.s, vqs1.s, t);

	return vqs;
}
