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

    RenderPass::Builder& RenderPass::Builder::AddAttachment(
        VkFormat format, 
        VkSampleCountFlagBits samples, 
        VkAttachmentLoadOp loadOp, 
        VkAttachmentStoreOp storeOp, 
        VkAttachmentLoadOp stencilLoadOp, 
        VkAttachmentStoreOp stencilStoreOp, 
        VkImageLayout initialLayout, 
        VkImageLayout finalLayout, 
        VkAttachmentDescriptionFlags flags)
    {
		VkAttachmentDescription desc{};
		desc.flags = flags;
		desc.format = format;
		desc.samples = samples;
		desc.loadOp = loadOp;
		desc.storeOp = storeOp;
		desc.stencilLoadOp = stencilLoadOp;
		desc.stencilStoreOp = stencilStoreOp;
		desc.initialLayout = initialLayout;
		desc.finalLayout = finalLayout;
		attachments_.push_back(desc);
		return *this;
    }

    RenderPass::Builder& RenderPass::Builder::AddColorAttachment(
        VkFormat format, VkSampleCountFlagBits samples, VkImageLayout finalLayout)
    {
		return AddAttachment(
			format,
			samples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			finalLayout
		);
    }

    RenderPass::Builder& RenderPass::Builder::AddDepthStencilAttachment(
        VkFormat format, VkSampleCountFlagBits samples, VkImageLayout finalLayout)
    {
		return AddAttachment(
			format,
			samples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			finalLayout
		);
    }

    RenderPass::Builder& RenderPass::Builder::BeginSubpass(VkPipelineBindPoint bindPoint)
    {
        SubpassConfig config{};
        config.bindPoint = bindPoint;
        subpasses_.push_back(config);
        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::AddColorAttachmentRef(uint32_t attachmentIndex, VkImageLayout layout)
    {
        VkAttachmentReference ref{};
        ref.attachment = attachmentIndex;
        ref.layout = layout;
        subpasses_.back().colorAttachments.push_back(ref);
    }

    RenderPass::Builder& RenderPass::Builder::SetDepthStencilAttachmentRef(uint32_t attachmentIndex, VkImageLayout layout)
    {
        subpasses_.back().depthStencilAttachment = { .attachment = attachmentIndex, .layout = layout };
        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::AddDependency(
        uint32_t srcSubpass, uint32_t dstSubpass, 
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, 
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
        VkDependencyFlags dependencyFlags)
    {
        VkSubpassDependency dep{};
		dep.srcSubpass = srcSubpass;
		dep.dstSubpass = dstSubpass;
		dep.srcStageMask = srcStageMask;
		dep.dstStageMask = dstStageMask;
		dep.srcAccessMask = srcAccessMask;
		dep.dstAccessMask = dstAccessMask;
		dep.dependencyFlags = dependencyFlags;
		dependencies_.push_back(dep);
        return *this;
    }

    RenderPass RenderPass::Builder::Build()
    {
        std::vector<VkSubpassDescription> subpassDescs(subpasses_.size(), VkSubpassDescription{});
        for (size_t i = 0; i < subpasses_.size(); ++i)
        {
            const auto& src = subpasses_[i];
            auto& dst = subpassDescs[i];

            dst.pipelineBindPoint = src.bindPoint;
            dst.colorAttachmentCount = static_cast<uint32_t>(src.colorAttachments.size());
            dst.pColorAttachments = src.colorAttachments.data();
            dst.pDepthStencilAttachment = &src.depthStencilAttachment;
        }

		VkRenderPassCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = static_cast<uint32_t>(attachments_.size());
		info.pAttachments = attachments_.empty() ? nullptr : attachments_.data();
		info.subpassCount = static_cast<uint32_t>(subpassDescs.size());
		info.pSubpasses = subpassDescs.data();
		info.dependencyCount = static_cast<uint32_t>(dependencies_.size());
		info.pDependencies = dependencies_.empty() ? nullptr : dependencies_.data();

        VkRenderPass renderPass = VK_NULL_HANDLE;
        if (vkCreateRenderPass(device_, &info, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");

        return RenderPass(renderPass);
    }
}