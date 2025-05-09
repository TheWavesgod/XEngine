#pragma once

#include "VkBase+.h"

#include "RenderPass.h"
#include "Framebuffer.h"
#include "Shader.h"

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
}
