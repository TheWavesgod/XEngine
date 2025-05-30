#pragma once

#include "Transform.h"
#include "Mesh.h"

#include <memory>

#include "glm.hpp"

#include "vulkan/Pipeline.h"


namespace LittleEngine
{
	using namespace glm;
	using namespace VK;

	class RenderObject
	{
	public:
		RenderObject(std::shared_ptr<Mesh> mesh) : mesh(mesh) {}

		void SetPosition(const vec3& newPos) { transform.SetPosition(newPos); }

		void Draw(VkCommandBuffer commandBuffer) const;

	protected:
		void Create();

		Transform transform;

		std::shared_ptr<Mesh> mesh;

		PipelineLayout pipelineLayout;
		Pipeline pipeline;
	};
}