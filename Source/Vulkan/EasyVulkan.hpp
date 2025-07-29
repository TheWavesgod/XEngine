#pragma once

#include "VkBase+.h"

#include "RenderPass.h"
#include "Framebuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"

#include "Attachment.h"

#include "MemoryBuffers.h"


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

    ColorAttachment ca_canvas;
    std::vector<DepthStencilAttachment> dsas_screenWithDS;

    const auto& CreateRpwf_Canvas(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT)
    {
        static renderPassWithFramebuffers rpwf;
        static VkFormat _depthStencilFormat = depthStencilFormat;

        VkAttachmentDescription attachmentDescriptions[2] = {
            { // color attachment
                .format = VkBase::Base().SwapchainCreateInfo().imageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE ,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            },
            { // depth stencil attachment
                .format = _depthStencilFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = _depthStencilFormat != VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            }
        };
        VkAttachmentReference attachmentReferences[2] = { 
            {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
            {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
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
            .pDependencies = &subpassDependency,
        };
        rpwf.renderPass.Create(renderPassCreateInfo);

        auto CreateFramebuffers = [] {
            dsas_screenWithDS.resize(VkBase::Base().SwapchainImageCount());
            rpwf.framebuffers.resize(VkBase::Base().SwapchainImageCount());
            for (auto& i : dsas_screenWithDS)
                i.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

            VkFramebufferCreateInfo framebufferCreateInfo = {
                .renderPass = rpwf.renderPass,
                .attachmentCount = 2,
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1};

            for (size_t i = 0; i < VkBase::Base().SwapchainImageCount(); ++i)
            {
                VkImageView attachments[2] = {
                    VkBase::Base().SwapchainImageView(i),
                    dsas_screenWithDS[i].ImageView()
                };
                framebufferCreateInfo.pAttachments = attachments;
                rpwf.framebuffers[i].Create(framebufferCreateInfo);
            }

        };
        auto DestroyFramebuffers = [] {
            dsas_screenWithDS.clear();
            rpwf.framebuffers.clear();
        };
        CreateFramebuffers();

        ExecuteOnce(rpwf); 
        VkBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
        VkBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);

        return rpwf;
    }
}
