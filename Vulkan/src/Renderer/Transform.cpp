

#include "Transform.h"

namespace LittleEngine
{
	void Transform::SetPosition(const glm::vec3& pos)
	{
		position = pos;
		updateTransformMatrix();
	}

	void Transform::SetRotation(const glm::vec3& eulerAngles)
	{
		rotation = glm::quat(glm::radians(eulerAngles));
		updateTransformMatrix();
	}

	void Transform::SetRotation(const glm::quat& quaternion)
	{
		rotation = quaternion;
		updateTransformMatrix();
	}

	void Transform::SetScale(const glm::vec3& scl)
	{
		scale = scl;
		updateTransformMatrix();
	}

	const Transform& Transform::operator*(const Transform& other) const
	{
		Transform result;
		result.position = position + rotation * (scale * other.position);
		result.rotation = rotation * other.rotation;
		result.scale = scale * other.scale;
		result.updateTransformMatrix();
		return result;
	}

	void Transform::updateTransformMatrix()
	{
		transformMatrix = glm::mat4(1.0f);
		transformMatrix = glm::translate(transformMatrix, position);
		transformMatrix *= glm::toMat4(rotation);
		transformMatrix = glm::scale(transformMatrix, scale);
	}
}