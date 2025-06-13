#include "Renderer.h"

#include "Scene.h"
#include "Camera.h"
#include "RenderObject.h"

#include "Vulkan/Window.h"
#include "vulkan/VkBase.h"
#include "vulkan/VkBase+.h"

namespace LittleEngine
{
	const VkExtent2D& windowSize = VkBase::Base().SwapchainCreateInfo().imageExtent;
	const VkClearValue clearColors[2] = {
			{ 0.0f, 0.1f, 0.1f, 1.f },
			{ 1.0f, 0.0f}
	};

	bool Renderer::Initialize()
	{
		for (size_t i = 0; i < VK::Window::GetPointer()->GetExtensionCount(); i++)
		{
			VK::VkBase::Base().Instance().AddExtension(VK::Window::GetPointer()->GetExtensionNames()[i]);
		}

#ifdef _WIN32
		VK::VkBase::Base().Instance().AddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
		VK::VkBase::Base().Instance().AddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else

#endif

		VK::VkBase::Base().UseLatestApiVersion();
		if (VK::VkBase::Base().CreateInstance()) return false;

		if (VK::Window::GetPointer()->CreateSuface()) return false;

		VK::VkBase::Base().Device().AddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (
			VK::VkBase::Base().SetPhysicalDevice(true, false) ||
			VK::VkBase::Base().CreateDevice())
		{
			return false;
		}

		if (VK::VkBase::Base().BuildSwapchain(VK::Window::GetPointer()->IsFrameRateLimited())) return false;

		CreateRenderPass();
		AllocateCommandBuffer();
		PrepareSynchronization();
		PrepareGlobalInfo();
	}

	void Renderer::RenderFrame(Scene& scene)
	{
		VkBase::Base().Swapchain().SwapImage(semaphore_imageIsAvailable);
		uint32_t i = VkBase::Base().Swapchain().CurrentImageIndex();

		const auto& [renderPass, framebuffers] = rpwf_swapChain;

		// Update Global Data
		viewDataBuffer.TransferData(scene.GetCam().GetCameraRenderingData());
		std::vector<RenderObject>& renderObjs = scene.GetRenderObjects();


		commandBuffer_Rendering.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer_Rendering, framebuffers[i], { {}, windowSize }, clearColors);
		{
			for (auto& obj : renderObjs)
			{
				obj.Draw(commandBuffer_Rendering);
			}
		}
		renderPass.CmdEnd(commandBuffer_Rendering);
		commandBuffer_Rendering.End();

		VkBase::Base().SubmitCommandBuffer_Graphics(commandBuffer_Rendering, semaphore_imageIsAvailable, semaphores_renderingIsOver[i], fence);
		VkBase::Base().Swapchain().PresentImage(semaphores_renderingIsOver[i]);

		fence.WaitAndReset();
	}

	bool Renderer::CreateRenderPass()
	{
		using namespace VK;

		VkAttachmentDescription attachmentDescriptions[2] = {
			{
				.format = VkBase::Base().SwapchainCreateInfo().imageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
			{
				.format = _depthStencilFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = _depthStencilFormat != VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
		};

		VkAttachmentReference attachmentReferences[2] = {
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};

		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = attachmentReferences,
			.pDepthStencilAttachment = attachmentReferences + 1
		};

		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 2,
			.pAttachments = attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};
		rpwf_swapChain.renderPass.Create(renderPassCreateInfo);

		// Create Render Attachment
		rpwf_swapChain.framebuffers.resize(VkBase::Base().SwapchainImageCount());
		dsas_screenWithDS.resize(VkBase::Base().SwapchainImageCount());

		for (auto& i : dsas_screenWithDS)
			i.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.renderPass = rpwf_swapChain.renderPass,
			.attachmentCount = 2,
			.width = windowSize.width,
			.height = windowSize.height,
			.layers = 1
		};

		for (size_t i = 0; i < VkBase::Base().SwapchainImageCount(); ++i) {
			VkImageView attachments[2] = {
				VkBase::Base().SwapchainImageView(i),
				dsas_screenWithDS[i].ImageView()
			};
			framebufferCreateInfo.pAttachments = attachments;
			rpwf_swapChain.framebuffers[i].Create(framebufferCreateInfo);
		}

		return true;
	}

	bool Renderer::AllocateCommandBuffer()
	{
		if (VkBase::Base().Plus().CommandPool_Graphics().AllocateBuffers(commandBuffer_Rendering)) return false;

		return true;
	}

	bool Renderer::PrepareSynchronization()
	{
		semaphore_imageIsAvailable.Create();
		semaphores_renderingIsOver = std::vector<Semaphore>(VkBase::Base().SwapchainImageCount());
		for (auto& semaphore : semaphores_renderingIsOver)
		{
			semaphore.Create();
		}
		fence.Create();

		return true;
	}

 	bool Renderer::PrepareGlobalInfo()
	{
		// Create Global Descriptor Set
		VkDescriptorPoolSize descriptorPoolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}
		};
		globalDescriptorPool.Create(2, descriptorPoolSizes);
		

		viewDataBuffer.Create(sizeof(Camera::RenderingData));

		// Initial view descriptor
		VkDescriptorSetLayoutBinding globalViewSetLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT };

		VkDescriptorSetLayoutCreateInfo globalViewSetLayoutCreateInfo = {
			.bindingCount = 1,
			.pBindings = &globalViewSetLayoutBinding
		};
		globalViewDescriptorSetLayout.Create(globalViewSetLayoutCreateInfo);

		globalDescriptorPool.AllocateSets(globalViewDescriptorSet, globalViewDescriptorSetLayout);

		VkDescriptorBufferInfo globalViewBufferInfo = {
			.buffer = viewDataBuffer,
			.offset = 0,
			.range = sizeof(Camera::RenderingData)
		};
		globalViewDescriptorSet.Write(globalViewBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);

		return true;
	}

}