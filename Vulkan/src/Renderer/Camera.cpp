#include "Camera.h"
#include "../Vulkan/Window.h"
#include <algorithm>

namespace LittleEngine
{
	using namespace VK;
	using namespace glm;

	inline glm::mat4 FlipVertical(const glm::mat4& proj)
	{
		glm::mat4 _projection = proj;
		for (uint32_t i = 0; i < 4; i++)
			_projection[i][1] *= -1;
		return _projection;
	}

	void Camera::Init()
	{
	}

	void Camera::Update(float dt)
	{
		// Update data in global camera buffer
		data = {
			.view = BuildViewMatrix(),
			.projection = BuildProjectionMatrix(),
			.camPos = transform.GetPosition()
		};
		globalCameraBuffer.TransferData(data); // TODO CHECK

		if (Window::enableMouseCursor) return;

		const float sensitive = 0.01f;
		
		yaw -= Window::mInputX * sensitive;
		pitch += Window::mInputY * sensitive;

		pitch = std::min(pitch, 89.0f);
		pitch = std::max(pitch, -89.0f);

		while (yaw < 0.0f)
			yaw += 360.0f;
		while (yaw > 360.0f)
			yaw -= 360.0f;

		transform.SetRotation(vec3(pitch, yaw, 0.0f));

		cameraMoveSpeed = std::clamp(cameraMoveSpeed + Window::mScroll * 0.5f, 0.0f, 50.0f);
		float displacement = cameraMoveSpeed * 0.1f * dt;

		vec3 moveDir = vec3(0.0f);
		if (Window::GetInputState(GLFW_KEY_W) == GLFW_PRESS) { moveDir += transform.GetForwardVector(); }
		if (Window::GetInputState(GLFW_KEY_S) == GLFW_PRESS) { moveDir -= transform.GetForwardVector(); }
		if (Window::GetInputState(GLFW_KEY_A) == GLFW_PRESS) { moveDir -= transform.GetRightVector(); }
		if (Window::GetInputState(GLFW_KEY_D) == GLFW_PRESS) { moveDir += transform.GetRightVector(); }
		if (Window::GetInputState(GLFW_KEY_E) == GLFW_PRESS) { moveDir += transform.GetUpVector(); }
		if (Window::GetInputState(GLFW_KEY_Q) == GLFW_PRESS) { moveDir -= transform.GetUpVector(); }
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
		return FlipVertical(glm::perspectiveFovRH_ZO(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
	}

	VkDescriptorSetLayoutBinding Camera::GetCameraGlobalDescriptorSetLayoutBinding() const
	{
		return {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
		};
	}
}

