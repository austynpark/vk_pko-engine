#include "vqs.h"

#include "math/pko_math.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <assimp/types.h>

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
	return rotate(q, r * s) + v;
}

VQS VQS::operator*(const VQS& vqs) const
{
	VQS result(*this, vqs);
	return result;
}

glm::mat4 VQS::to_matrix() const
{	
	return glm::translate(glm::mat4(1.0f), glm::vec3(v.x, v.y, v.z)) * quat_to_matrix(q) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

VQS to_vqs(const aiMatrix4x4& mat)
{
	aiVector3D translation;
	aiVector3D scale;
	aiQuaternion rotation;

	mat.Decompose(scale, rotation, translation);

	//TODO: not sure if scale value is adequate
	VQS vqs(to_vec3(translation), to_quat(rotation), std::min(std::min(scale.x, scale.y), scale.z));

	return vqs;
}

VQS to_vqs(const glm::mat4& mat)
{
	glm::vec3 translation;
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mat, scale, rotation, translation, skew, perspective);

	VQS vqs(to_vec3(translation), to_quat(rotation), std::max(std::max(scale.x, scale.y), scale.z));

	return vqs;
}

VQS interpolate_vqs(const VQS& vqs0, const VQS& vqs1, f32 t)
{
	VQS vqs;

	vqs.v = lerp(vqs0.v, vqs1.v, t);
	vqs.q = slerp(vqs0.q, vqs1.q, t);
	vqs.s =	elerp(vqs0.s, vqs1.s, t);

	return vqs;
}
