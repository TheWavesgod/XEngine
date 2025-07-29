#include "RenderPass.h"
#include "VkBase.h"

namespace VK
{
    RenderPass::~RenderPass()
    {
        DestroyHandleBy(vkDestroyRenderPass);
    }

    void RenderPass::CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents) const
    {
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = handle;
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
    }

    void RenderPass::CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea, 
        arrayRef<const VkClearValue> clearValues, VkSubpassContents subpassContents) const 
    {
        VkRenderPassBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = handle,
            .framebuffer = framebuffer,
            .renderArea = renderArea,
            .clearValueCount = uint32_t(clearValues.Count()),
            .pClearValues = clearValues.Pointer()
        };
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
    }

    result_t RenderPass::Create(VkRenderPassCreateInfo& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        VkResult result = vkCreateRenderPass(VkBase::Base().Device(), &createInfo, nullptr, &handle);
        if (result)
            outStream << std::format("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
        return result;
    }
}