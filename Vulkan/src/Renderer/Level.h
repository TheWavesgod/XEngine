#pragma once

#include <vector>

#include "RenderObject.h"
#include "Camera.h"

#include "../Vulkan/VkBase+.h"
#include "../Vulkan/RenderPass.h"
#include "../Vulkan/Framebuffer.h"

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
		std::vector<RenderObject> renderObjects;
		Camera cam;

		CommandBuffer commandBuffer;
		CommandPool commandPool;

		Semaphore semaphore_imageIsAvailable;
		std::vector<Semaphore> semaphores_renderingIsOver;
		Fence fence;

		const VkExtent2D& windowSize = VkBase::Base().SwapchainCreateInfo().imageExtent;

		struct renderPassWithFramebuffers
		{
			RenderPass renderPass;
			std::vector<Framebuffer> framebuffers;
		} rpwf_swapChain;

	private:
		void InitialScene();
	};
}


