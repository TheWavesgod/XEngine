#pragma once

#include "VKEasyHeader.h"

namespace VK
{
    class RenderPass
    {
        VkRenderPass handle = VK_NULL_HANDLE;

	public:
		RenderPass() = default;
		RenderPass(VkRenderPass renderPass) : handle(renderPass) {}
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

    public:
        class Builder
        {
        public:
			explicit Builder(VkDevice device) : device_(device) {}

            Builder& AddAttachment(
                VkFormat format,
                VkSampleCountFlagBits samples,
                VkAttachmentLoadOp loadOp,
                VkAttachmentStoreOp storeOp,
                VkAttachmentLoadOp stencilLoadOp,
                VkAttachmentStoreOp stencilStoreOp,
                VkImageLayout initialLayout,
                VkImageLayout finalLayout,
                VkAttachmentDescriptionFlags flags = 0);

            Builder& AddColorAttachment(
                VkFormat format,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

            Builder& AddDepthStencilAttachment(
                VkFormat format,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            // Subpass build 
            Builder& BeginSubpass(VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

            Builder& AddColorAttachmentRef(
                uint32_t attachmentIndex,
                VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            Builder& SetDepthStencilAttachmentRef(
                uint32_t attachmentIndex,
                VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            // Dependency part
            Builder& AddDependency(
                uint32_t srcSubpass,
                uint32_t dstSubpass,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkDependencyFlags dependencyFlags = 0);

            RenderPass Build();

        private:
			struct SubpassConfig 
            {
				VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				std::vector<VkAttachmentReference> colorAttachments;
				VkAttachmentReference depthStencilAttachment;
			};

            VkDevice device_;
            std::vector<VkAttachmentDescription> attachments_;
            std::vector<SubpassConfig> subpasses_;
            std::vector<VkSubpassDependency> dependencies_;
        };
    };
}
