#pragma once

#include <vector>

#include "Mesh.h"

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

	private:
		std::vector<Mesh> renderObjects;

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
	};
}


