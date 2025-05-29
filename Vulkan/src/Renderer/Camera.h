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
		Camera(void)
		{
			yaw = 90.0f;
			pitch = 0.0f;
			cameraMoveSpeed = 30.0f;
			transform.SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
		}

		Camera(float pitch, float yaw, glm::vec3 position)
		{
			this->pitch = pitch;
			this->yaw = yaw;
			this->cameraMoveSpeed = 30.0f;
			transform.SetPosition(position);
		}

		~Camera(void) {}

		void Init();
		void Update(float dt = 1.0f);

		glm::mat4 BuildViewMatrix();
		glm::mat4 BuildProjectionMatrix();

		float GetCameraYaw() const { return yaw; }
		void SetCameraYaw(const float& y) { yaw = y; }

		float GetCameraPitch() const { return pitch; }
		void SetCameraPitch(const float& p) { pitch = p; }

		inline Transform& GetCameraTransform() { return transform; }

		const UniformBuffer& GetGlobalCameraBuffer() const { return globalCameraBuffer; }
		VkDescriptorSetLayoutBinding GetCameraGlobalDescriptorSetLayoutBinding() const;

		// For Renderer Data
		struct GlobalCameraData
		{
			mat4 view;
			mat4 projection;
			vec3 camPos;
			float pad0;
		};

	protected:
		float yaw;
		float pitch;
		float cameraMoveSpeed;

		Transform transform;

		

		UniformBuffer globalCameraBuffer = UniformBuffer(sizeof GlobalCameraData);
	};
}