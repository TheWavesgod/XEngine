#include "Level.h"

#include "vulkan/Shader.h"

namespace LittleEngine
{
	using namespace VK;

	void Level::Initialize()
	{
		static VkFormat _depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

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
		dsas_screenWithDS.resize(VkBase::Base().SwapchainImageCount());
		rpwf_swapChain.framebuffers.resize(VkBase::Base().SwapchainImageCount());
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

		commandPool.Create(VK::VkBase::Base().PhysicalDevice().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		commandPool.AllocateBuffers(commandBuffer);

		semaphores_renderingIsOver = std::vector<Semaphore>(VkBase::Base().SwapchainImageCount());

		// Create Global Descriptor Set
		VkDescriptorPoolSize descriptorPoolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
		};
		descriptorPool.Create(1, descriptorPoolSizes);

		VkDescriptorSetLayoutBinding globalCamUBDB = cam.GetCameraGlobalDescriptorSetLayoutBinding();

		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &globalCamUBDB
		};
		globalUniformDescriptorSetLayout.Create(layoutInfo);

		descriptorPool.AllocateSets(globalUniformDescriptorSet, globalUniformDescriptorSetLayout);

		VkDescriptorBufferInfo bufferInfo = {
			.buffer = cam.GetGlobalCameraBuffer(),
			.offset = 0,
			.range = sizeof Camera::GlobalCameraData
		};

		globalUniformDescriptorSet.Write(bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		InitialScene();
	}

	void Level::RenderFrame()
	{
		VkBase::Base().Swapchain().SwapImage(semaphore_imageIsAvailable);
		uint32_t i = VkBase::Base().Swapchain().CurrentImageIndex();

		const auto& [renderPass, framebuffers]= rpwf_swapChain;

		VkClearValue clearColors[2] = {
			{ 0.0f, 0.1f, 0.1f, 1.f },
			{ 1.0f, 0.0f}
		};

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColors);
		{
			for (const RenderObject& ro : renderObjects)
			{
				ro.Draw(commandBuffer, globalUniformDescriptorSet);
			}
		}
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		VkBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphores_renderingIsOver[i], fence);
		VkBase::Base().Swapchain().PresentImage(semaphores_renderingIsOver[i]);

		fence.WaitAndReset();
	}

	void Level::UpdateObject(float dt)
	{
		cam.Update(dt);
	}

	void Level::InitialScene()
	{
		cam.GetCameraTransform().SetPosition({ 0.0f, 0.0f, 3.0f });
		cam.SetCameraYaw(0.0f);

		renderObjects.emplace_back(Mesh::GenerateCube());
		renderObjects.emplace_back(Mesh::GenerateCube());
		renderObjects.emplace_back(Mesh::GenerateCube());

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			renderObjects[i].Create(rpwf_swapChain.renderPass, globalUniformDescriptorSetLayout);
			renderObjects[i].SetPosition({ -3.0f + (float)(i * 2), 0.0f, -5.0f });
		}
	}
}
