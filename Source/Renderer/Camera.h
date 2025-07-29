#pragma once

#include "Transform.h"

#include "Vulkan/MemoryBuffers.h"
#include "Vulkan/Descriptor.h"

namespace LittleEngine
{
	using namespace VK;
	using namespace glm;

	class Camera
	{
	public:
		struct RenderingData
		{
			mat4 view;
			mat4 projection;
			vec3 camPos;
			float pad0;
		};

		Camera(void)
		{
			yaw = 90.0f;
			pitch = 0.0f;
			cameraMoveSpeed = 10.0f;
			transform.SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
		}

		Camera(float pitch, float yaw, glm::vec3 position)
		{
			this->pitch = pitch;
			this->yaw = yaw;
			this->cameraMoveSpeed = 10.0f;
			transform.SetPosition(position);
		}

		~Camera(void) {}

		void Init();
		void Update(float dt = 1.0f);

		glm::mat4 BuildViewMatrix();
		glm::mat4 BuildProjectionMatrix();

		float GetCameraYaw() const { return yaw; }
		void SetCameraYaw(const float& y) { yaw = y; transform.SetRotation(vec3(pitch, yaw, 0.0f)); }

		float GetCameraPitch() const { return pitch; }
		void SetCameraPitch(const float& p) { pitch = p; transform.SetRotation(vec3(pitch, yaw, 0.0f)); }

		inline Transform& GetCameraTransform() { return transform; }

		RenderingData GetCameraRenderingData();

	protected:
		float yaw;
		float pitch;
		float cameraMoveSpeed;

		Transform transform;
	};
}