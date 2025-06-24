#include "Renderer.h"

#include "Scene.h"
#include "Camera.h"
#include "RenderObject.h"

#include "Vulkan/Window.h"
#include "vulkan/VkBase.h"
#include "vulkan/VkBase+.h"
#include "vulkan/Texture.h"

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
		
		PrepareSynchronization();
		BootScreen("../Resources/Images/StartupImage.png", VK_FORMAT_R8G8B8A8_UNORM);

		CreateRenderPass();
		AllocateCommandBuffer();
		PrepareGlobalInfo();
	}

	void Renderer::RenderFrame(Scene& scene)
	{
		VkBase::Base().Swapchain().SwapImage(semaphore_imageIsAvailable);
		const uint32_t i = VkBase::Base().Swapchain().CurrentImageIndex();

		const auto& [rp_draw, fb_draw] = rpwf_draw;
		const auto& [rp_postProcess, fbs_postProcess] = rpwf_postProcess;

		// Update Global Data
		viewDataBuffer.TransferData(scene.GetCam().GetCameraRenderingData());
		std::vector<RenderObject>& renderObjs = scene.GetRenderObjects();

		commandBuffer_Rendering.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		// Draw scene
		rp_draw.CmdBegin(commandBuffer_Rendering, fb_draw, { {}, windowSize }, clearColors);
		{
			for (auto& obj : renderObjs)
			{
				obj.Draw(commandBuffer_Rendering);
			}
		}
		rp_draw.CmdEnd(commandBuffer_Rendering);

		// Post process
		rp_postProcess.CmdBegin(commandBuffer_Rendering, fbs_postProcess[i], { {}, windowSize }, clearColors[0]);
		{
			vkCmdBindPipeline(commandBuffer_Rendering, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_postProcess);
			vkCmdBindDescriptorSets(commandBuffer_Rendering, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_postProcess,
				0, 1, postProcessDescriptorSet.Address(), 0, nullptr);
			vkCmdPushConstants(commandBuffer_Rendering, pipelineLayout_postProcess, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &settings.exposure);
			vkCmdDraw(commandBuffer_Rendering, 3, 1, 0, 0);
		}
		rp_postProcess.CmdEnd(commandBuffer_Rendering);
		
		commandBuffer_Rendering.End();

		VkBase::Base().SubmitCommandBuffer_Graphics(commandBuffer_Rendering, semaphore_imageIsAvailable, semaphores_renderingIsOver[i], fence);
		VkBase::Base().Swapchain().PresentImage(semaphores_renderingIsOver[i]);

		fence.WaitAndReset();
	}

	void Renderer::BootScreen(const std::string& imagePath, VkFormat imageFormat)
	{
		VkExtent2D imageExtent;
		std::unique_ptr<uint8_t[]> pImageData = Texture2d::LoadFile(imagePath.c_str(), imageExtent, FormatInfo(imageFormat));
		if (!pImageData)
		{
			outStream << std::format("[ BootScreen ] ERROR\nFailed to load image file!\n");
			return;
		}
		StagingBuffer::BufferData_MainThread(pImageData.get(), FormatInfo(imageFormat).sizePerPixel * imageExtent.width * imageExtent.height);
		
		CommandBuffer commandBuffer;
		VkBase::Plus().CommandPool_Graphics().AllocateBuffers(commandBuffer);
		
		VkBase::Base().Swapchain().SwapImage(semaphore_imageIsAvailable);

		ImageMemory imageMemory;
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			VkExtent2D swapchainImageSize = VkBase::Base().SwapchainCreateInfo().imageExtent;

			bool useBlit =
				imageExtent.width != swapchainImageSize.width ||
				imageExtent.height != swapchainImageSize.height ||
				imageFormat != VkBase::Base().SwapchainCreateInfo().imageFormat;

			if (useBlit)
			{
				VkImage image = StagingBuffer::AliasedImage2d_MainThread(imageFormat, imageExtent);
				if (image)
				{
					VkImageMemoryBarrier imageMemoryBarrier = {
					   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					   nullptr,
					   0,
					   VK_ACCESS_TRANSFER_READ_BIT,
					   VK_IMAGE_LAYOUT_PREINITIALIZED,
					   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   VK_QUEUE_FAMILY_IGNORED,
					   VK_QUEUE_FAMILY_IGNORED,
					   image,
					   { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
					};
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
				else
				{
					VkImageCreateInfo imageCreateInfo = {
						.imageType = VK_IMAGE_TYPE_2D,
						.format = imageFormat,
						.extent = { imageExtent.width, imageExtent.height, 1 },
						.mipLevels = 1,
						.arrayLayers = 1,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
					};
					imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

					VkBufferImageCopy region_copy = {
						.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
						.imageExtent = imageCreateInfo.extent
					};

					ImageOperation::CmdCopyBufferToImage(commandBuffer,
						StagingBuffer::Buffer_MainThread(),
						imageMemory.ImageRef(),
						region_copy,
						{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
						{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL });

					image = imageMemory.ImageRef();
				}

				VkImageBlit region_blit = {
				  { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				  { {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
				  { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				  { {}, { int32_t(swapchainImageSize.width), int32_t(swapchainImageSize.height), 1 } }
				};

				ImageOperation::CmdBlitImage(commandBuffer,
					image,
					VkBase::Base().SwapchainImage(VkBase::Base().Swapchain().CurrentImageIndex()),
					region_blit,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
					{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }, VK_FILTER_LINEAR
				);
			}
			else
			{
				VkBufferImageCopy region_copy = {
					.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					.imageExtent = { imageExtent.width, imageExtent.height, 1 }
				};

				ImageOperation::CmdCopyBufferToImage(commandBuffer,
					StagingBuffer::Buffer_MainThread(),
					VkBase::Base().SwapchainImage(VkBase::Base().Swapchain().CurrentImageIndex()),
					region_copy,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
					{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
				);
			}
		}
		commandBuffer.End();

		// submit command buffer
		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = semaphore_imageIsAvailable.Address(),
			.pWaitDstStageMask = &waitDstStage,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffer.Address()
		};
		VkBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
		// wait until the commands is completed
		fence.WaitAndReset();
		VkBase::Base().Swapchain().PresentImage();
		// don't forget to release the command buffer
		VkBase::Plus().CommandPool_Graphics().FreeBuffers(commandBuffer);
	}

	bool Renderer::CreateRenderPass()
	{
		using namespace VK;

		VkAttachmentDescription attachmentDescriptions_draw[2] = {
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT, // HDR format
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{
				.format = _depthStencilFormat,
				.loadOp = _depthStencilFormat != VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
		};

		VkAttachmentReference colorAttachRef = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference depthAttachRef = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass_draw = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachRef,
			.pDepthStencilAttachment = &depthAttachRef
		};

		VkSubpassDependency subpassDependency_draw = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		VkRenderPassCreateInfo rpCreateInfo_draw = {
			.attachmentCount = 2,
			.pAttachments = attachmentDescriptions_draw,
			.subpassCount = 1,
			.pSubpasses = &subpass_draw,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency_draw
		};
		rpwf_draw.renderPass.Create(rpCreateInfo_draw);

		ca_draw.Create(VK_FORMAT_R16G16B16A16_SFLOAT, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		dsa_draw.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		VkImageView attachments_draw[] = {ca_draw.ImageView(), dsa_draw.ImageView()};

		VkFramebufferCreateInfo fbCreateInfo_draw = {
			.renderPass = rpwf_draw.renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments_draw,
			.width = windowSize.width,
			.height = windowSize.height,
			.layers = 1
		};
		rpwf_draw.frameBuffer.Create(fbCreateInfo_draw);

		// Post process Render pass
		VkAttachmentDescription attachmentDescription_postProcess = {
			.format = VkBase::Base().SwapchainCreateInfo().imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		VkAttachmentReference attachRef_postProcess = {
			0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass_postProcess = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachRef_postProcess,
		};

		VkSubpassDependency subpassDependency_postProcess = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 1,
			.pAttachments = &attachmentDescription_postProcess,
			.subpassCount = 1,
			.pSubpasses = &subpass_postProcess,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency_postProcess
		};
		rpwf_postProcess.renderPass.Create(renderPassCreateInfo);

		// Create Render Attachment
		rpwf_postProcess.framebuffers.resize(VkBase::Base().SwapchainImageCount());

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.renderPass = rpwf_postProcess.renderPass,
			.attachmentCount = 1,
			.width = windowSize.width,
			.height = windowSize.height,
			.layers = 1
		};

		for (size_t i = 0; i < VkBase::Base().SwapchainImageCount(); ++i) {
			VkImageView attachment = VkBase::Base().SwapchainImageView(i);
			framebufferCreateInfo.pAttachments = &attachment;
			rpwf_postProcess.framebuffers[i].Create(framebufferCreateInfo);
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
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20},
			{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 20},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20}
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