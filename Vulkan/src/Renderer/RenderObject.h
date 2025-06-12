#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

#include <memory>

#include "glm.hpp"

#include "vulkan/Pipeline.h"

namespace VK
{
	class DescriptorSet;
	class RenderPass;
	class DescriptorSetLayout;
}

namespace LittleEngine
{
	using namespace glm;
	using namespace VK;

	class RenderObject
	{
	public:
		RenderObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material = nullptr) : mesh(mesh), material(material) { }

		void SetPosition(const vec3& newPos) { transform.SetPosition(newPos); }

		void Draw(VkCommandBuffer commandBuffer) const;

		void Create();
	
	protected:
		Transform transform;

		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;

		PipelineLayout pipelineLayout;
		Pipeline pipeline;
	};
}