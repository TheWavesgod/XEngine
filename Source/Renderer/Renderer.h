#pragma once

#include "Vulkan/RenderPass.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/Attachment.h"
#include "Vulkan/Descriptor.h"
#include "Vulkan/Command.h"
#include "Vulkan/Synchronization.h"
#include "Vulkan/MemoryBuffers.h"

namespace LittleEngine
{
	using namespace VK;

	class Scene;

	struct RendererSettings
	{
		float exposure = 1.0f;
	};

	class Renderer
	{
		static Renderer renderer;

		Renderer() = default;
		Renderer(Renderer&&) = delete;

		static inline  VkFormat _depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	public:
		static Renderer& Get() { return renderer; }

		bool Initialize();

		void RenderFrame(Scene& scene);

		void BootScreen(const std::string& imagePath, VkFormat imageFormat);

		bool CreateRenderPass();
		bool AllocateCommandBuffer();
		bool PrepareSynchronization();
		bool PrepareGlobalInfo();
		
		// Getter
		DescriptorPool& GetGlobalDescriptorPool() { return globalDescriptorPool; }
		const DescriptorSet& GetViewDescriptorSet() const { return globalViewDescriptorSet; }
		const DescriptorSetLayout& GetViewDescriptorSetLayout() const { return globalViewDescriptorSetLayout; }

		RenderPass& GetCurrentRenderPass() { return rpwf_draw.renderPass; }

		RendererSettings settings;

	private:
		struct RenderPassWithFramebuffers
		{
			RenderPass renderPass;
			std::vector<Framebuffer> framebuffers;
		} rpwf_postProcess;
		std::vector<DepthStencilAttachment> dsas_postProcess;

		struct RenderPassWithFramebuffer
		{
			RenderPass renderPass;
			Framebuffer frameBuffer;
		} rpwf_draw;
		ColorAttachment ca_draw;
		DepthStencilAttachment dsa_draw;

		Pipeline pipeline_postProcess;
		PipelineLayout pipelineLayout_postProcess;

		CommandBuffer commandBuffer_Rendering;

		Semaphore semaphore_imageIsAvailable;
		std::vector<Semaphore> semaphores_renderingIsOver;
		Fence fence;

		DescriptorPool globalDescriptorPool;

		// Global Info
		DescriptorSet globalViewDescriptorSet;
		DescriptorSetLayout globalViewDescriptorSetLayout;

		DescriptorSet postProcessDescriptorSet;
		DescriptorSetLayout postProcessDescriptorSetLayout;

		UniformBuffer viewDataBuffer;
	};

	inline Renderer Renderer::renderer;
}