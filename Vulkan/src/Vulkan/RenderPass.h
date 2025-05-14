#pragma once

#include "VKEasyHeader.h"

namespace VK
{
    class RenderPass
    {
        VkRenderPass handle = VK_NULL_HANDLE;

    public:
        RenderPass() = default;
        RenderPass(VkRenderPassCreateInfo& createInfo) { Create(createInfo); }

        RenderPass(RenderPass&& other) noexcept { MoveHandle; }
        ~RenderPass();

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Const Function
        void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const;

        void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, 
            VkRect2D renderArea, arrayRef<const VkClearValue> clearValues = {}, 
            VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const;


        void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
            vkCmdNextSubpass(commandBuffer, subpassContents);
        }

        void CmdEnd(VkCommandBuffer commandBuffer) const {
            vkCmdEndRenderPass(commandBuffer);
        }

        //Non-const Function
        result_t Create(VkRenderPassCreateInfo& createInfo);
    };

}
