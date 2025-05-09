#pragma once

#include "VkBase.h"

namespace VK
{
    class RenderPass
    {
        VkRenderPass handle = VK_NULL_HANDLE;

    public:
        RenderPass() = default;
        RenderPass(VkRenderPassCreateInfo& createInfo) { Create(createInfo); }
        RenderPass(RenderPass&& other) noexcept { MoveHandle; }
        ~RenderPass() { DestroyHandleBy(vkDestroyRenderPass); }

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Const Function
        void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = handle;
            vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
        }

        void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea, arrayRef<const VkClearValue> clearValues = {}, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
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

        void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
            vkCmdNextSubpass(commandBuffer, subpassContents);
        }

        void CmdEnd(VkCommandBuffer commandBuffer) const {
            vkCmdEndRenderPass(commandBuffer);
        }

        //Non-const Function
        result_t Create(VkRenderPassCreateInfo& createInfo)
        {
            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            VkResult result = vkCreateRenderPass(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

}
