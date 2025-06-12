#pragma once

#include "Vulkan/RenderPass.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/Attachment.h"
#include "Vulkan/Descriptor.h"
#include "Vulkan/Command.h"
#include "Vulkan/Synchronization.h"

namespace LittleEngine
{
	using namespace VK;

	class Renderer
	{
		static Renderer renderer;

		Renderer() = default;
		Renderer(Renderer&&) = delete;

		static inline  VkFormat _depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	public:
		static Renderer& Get() { return renderer; }

		bool Initialize();

		void RenderFrame();

		bool CreateRenderPass();
		bool AllocateCommandBuffer();
		bool PrepareSynchronization();
		bool CreateDescriptor();
		
		// Getter
		DescriptorPool& GetGlobalDescriptorPool() { return globalDescriptorPool; }

	private:
		struct renderPassWithFramebuffers
		{
			RenderPass renderPass;
			std::vector<Framebuffer> framebuffers;
		} rpwf_swapChain;
		std::vector<DepthStencilAttachment> dsas_screenWithDS;

		CommandBuffer commandBuffer_Rendering;

		Semaphore semaphore_imageIsAvailable;
		std::vector<Semaphore> semaphores_renderingIsOver;
		Fence fence;

		DescriptorPool globalDescriptorPool;

		DescriptorSet globalUniformDescriptorSet;
		DescriptorSetLayout globalUniformDescriptorSetLayout;
	};

	inline Renderer Renderer::renderer;
}