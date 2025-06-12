#pragma once

#include <vector>

#include "RenderObject.h"
#include "Camera.h"

#include "Vulkan/VkBase+.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/Descriptor.h"
#include "Vulkan/Attachment.h"

namespace LittleEngine
{
	using namespace VK;

	class Level
	{
	public:

		void Initialize();

		void UpdateObject(float dt);

	private:
		Camera cam;
		std::vector<RenderObject> renderObjects;
		
		std::vector<std::shared_ptr<Material>> materials;

		DescriptorSetLayout globalUniformDescriptorSetLayout; 
		DescriptorSet globalUniformDescriptorSet;

	private:
		void InitialScene();
	};
}


