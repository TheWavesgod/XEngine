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
		void RenderFrame();
		void UpdateObject(float dt);

	private:
		Camera cam;
		std::vector<RenderObject> renderObjects;
		
		std::vector<std::shared_ptr<Material>> materials;
		
		CommandBuffer commandBuffer;
		CommandPool commandPool;

		DescriptorPool descriptorPool;
		DescriptorSetLayout globalUniformDescriptorSetLayout; 
		DescriptorSet globalUniformDescriptorSet;

		Semaphore semaphore_imageIsAvailable;
		std::vector<Semaphore> semaphores_renderingIsOver;
		Fence fence;

		const VkExtent2D& windowSize = VkBase::Base().SwapchainCreateInfo().imageExtent;

		struct renderPassWithFramebuffers
		{
			RenderPass renderPass;
			std::vector<Framebuffer> framebuffers;
		} rpwf_swapChain;

		std::vector<DepthStencilAttachment> dsas_screenWithDS;

	private:
		void InitialScene();
	};
}


