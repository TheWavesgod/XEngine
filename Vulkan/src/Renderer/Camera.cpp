#include "Camera.h"
#include "../Vulkan/Window.h"
#include <algorithm>

namespace LittleEngine
{
	using namespace VK;
	using namespace glm;

	void Camera::UpdateCamera(float dt)
	{
		if (Window::enableMouseCursor) return;

		const float sensitive = 0.1f;
		
		yaw -= Window::mInputX * sensitive;
		pitch += Window::mInputY * sensitive;

		pitch = std::min(pitch, 89.0f);
		pitch = std::max(pitch, -89.0f);

		while (yaw < 0.0f)
			yaw += 360.0f;
		while (yaw > 360.0f)
			yaw -= 360.0f;

		transform.SetRotation(vec3(pitch, yaw, 0.0f));

		cameraMoveSpeed = std::clamp(cameraMoveSpeed + Window::mScroll * 0.05f, 3.0f, 50.0f);
		float displacement = cameraMoveSpeed * dt;

		vec3 moveDir = vec3(0.0f);
		if (Window::GetInputState(GLFW_KEY_W) == GLFW_PRESS) { moveDir += transform.GetForwardVector(); }
		if (Window::GetInputState(GLFW_KEY_S) == GLFW_PRESS) { moveDir += -transform.GetForwardVector(); }
		if (Window::GetInputState(GLFW_KEY_A) == GLFW_PRESS) { moveDir += -transform.GetRightVector(); }
		if (Window::GetInputState(GLFW_KEY_D) == GLFW_PRESS) { moveDir += transform.GetRightVector(); }
		if (length(moveDir) != 0.0f)
		{
			vec3 moveDistance = glm::normalize(moveDir) * displacement;
			transform.SetPosition(transform.GetPosition() + moveDistance);
		}
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

