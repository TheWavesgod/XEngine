#include "Camera.h"

namespace LittleEngine
{
	void Camera::UpdateCamera(float dt)
	{

	}

	glm::mat4 Camera::BuildViewMatrix()
	{
		return glm::lookAt(transform.GetPosition(), transform.GetPosition() + transform.GetForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::mat4 Camera::BuildProjectionMatrix()
	{
		return glm::perspective(glm::radians(45.0f), float(1280 / 720), 0.1f, 1000.0f);
	}
}

