#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

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
		RenderObject(Mesh* mesh, Material* material = nullptr) : mesh(mesh), material(material) { }

		void SetPosition(const vec3& newPos) { transform.SetPosition(newPos); }

		void Draw(VkCommandBuffer commandBuffer) const;

		void Create();
	
	protected:
		Transform transform;

		Mesh* mesh;
		Material* material;

		PipelineLayout pipelineLayout;
		Pipeline pipeline;
	};
}