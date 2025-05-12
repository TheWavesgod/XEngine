#pragma once

#include "VkBase+.h"

#include "RenderPass.h"
#include "Framebuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"

using namespace VK;

inline const VkExtent2D& windowSize = VkBase::Base().SwapchainCreateInfo().imageExtent;

namespace EasyVulkan 
{
    using namespace VK;

    struct renderPassWithFramebuffers
    {
        RenderPass renderPass;
        std::vector<Framebuffer> framebuffers;
    };

    const renderPassWithFramebuffers& CreateRpwf_Screen()
    {
        static renderPassWithFramebuffers rpwf;

        VkAttachmentDescription attachmentDescription = {
            .format = VkBase::Base().SwapchainCreateInfo().imageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentReference
        };

        VkSubpassDependency subpassDependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,//不早于提交命令缓冲区时等待semaphore对应的waitDstStageMask
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        };

        VkRenderPassCreateInfo renderPassCreateInfo = {
            .attachmentCount = 1,
            .pAttachments = &attachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = &subpassDependency
        };
        rpwf.renderPass.Create(renderPassCreateInfo);

        auto CreateFramebuffers = []()
        {
            rpwf.framebuffers.resize(VkBase::Base().SwapchainImageCount());
            VkFramebufferCreateInfo framebufferCreateInfo = {
                .renderPass = rpwf.renderPass,
                .attachmentCount = 1,
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1
            };
            for (size_t i = 0; i < VkBase::Base().SwapchainImageCount(); ++i)
            {
                VkImageView attachment = VkBase::Base().SwapchainImageView(i);
                framebufferCreateInfo.pAttachments = &attachment;
                rpwf.framebuffers[i].Create(framebufferCreateInfo);
            }
        };

        auto DestroyFramebuffers = []()
        {
            rpwf.framebuffers.clear();
        };
        
        CreateFramebuffers();
        
        ExecuteOnce(rpwf); //防止再次调用本函数时，重复添加回调函数
        VkBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
        VkBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
        
        return rpwf;
    }

    void BootScreen(const char* imagePath, VkFormat imageFormat)
    {
        VkExtent2D imageExtent;

        std::unique_ptr<uint8_t[]> pImageData = Texture2d::LoadFile(imagePath, imageExtent, FormatInfo(imageFormat));
        if (!pImageData)
            return;

        StagingBuffer::BufferData_MainThread(pImageData.get(), FormatInfo(imageFormat).sizePerPixel * imageExtent.width * imageExtent.height);

        Semaphore semaphore_imageIsAvailable;
        Fence fence;

        // Allocate the command buffer
        CommandBuffer commandBuffer;
        VkBase::Plus().CommandPool_Graphics().AllocateBuffers(commandBuffer);

        // Acquire the swapchain images
        VkBase::Base().SwapImage(semaphore_imageIsAvailable);

        // record command buffer
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            VkExtent2D swapchainImageSize = VkBase::Base().SwapchainCreateInfo().imageExtent;
            bool blit =
                imageExtent.width != swapchainImageSize.width ||                       
                imageExtent.height != swapchainImageSize.height ||                     
                imageFormat != VkBase::Base().SwapchainCreateInfo().imageFormat; 

            ImageMemory imageMemory;
            if (blit)
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

                    //将image赋值为imageMemory.Image()以便后续操作
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
                    VkBase::Base().SwapchainImage(VkBase::Base().CurrentImageIndex()),
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
                    VkBase::Base().SwapchainImage(VkBase::Base().CurrentImageIndex()),
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
        // present the iamge
        VkBase::Base().PresentImage();

        // don't forget to release the command buffer
        VkBase::Plus().CommandPool_Graphics().FreeBuffers(commandBuffer);
    }
}
