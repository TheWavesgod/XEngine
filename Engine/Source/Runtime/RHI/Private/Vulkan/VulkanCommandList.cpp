#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanPipeline.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanCheckedCast.h"

#include <XEngine/Logging/Log.h>

#include <algorithm>

namespace XEngine
{
    namespace
    {
        VkPipelineStageFlags GetSourceStage(VkImageLayout layout)
        {
            switch (layout)
            {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            default:
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
        }

        VkAccessFlags GetSourceAccess(VkImageLayout layout)
        {
            switch (layout)
            {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            default:
                return 0;
            }
        }
    }

    void VulkanCommandList::BeginFrame(
        VulkanDevice& device,
        VkCommandBuffer commandBuffer,
        VkImage swapchainImage,
        VkImageView swapchainImageView,
        VkExtent2D swapchainExtent,
        VkImageLayout* swapchainImageLayout,
        VulkanTexture* depthTexture)
    {
        m_Device = &device;   
        m_CommandBuffer = commandBuffer;
        m_SwapchainImage = swapchainImage;
        m_SwapchainImageView = swapchainImageView;
        m_SwapchainExtent = swapchainExtent;
        m_RenderViewport = RHIRect2D { 0, 0, swapchainExtent.width, swapchainExtent.height };
        m_RenderOutput = RHIRenderOutputDesc {};
        m_RenderOutput.Viewport = m_RenderViewport;
        m_RenderOutput.RenderToSwapchain = true;
        m_SwapchainImageLayout = swapchainImageLayout;
        m_DepthTexture = depthTexture;
        m_BoundGraphicsPipeline = nullptr;
        m_RenderingActive = false;
    }

    void VulkanCommandList::Reset()
    {
        m_Device = nullptr;  
        m_CommandBuffer = VK_NULL_HANDLE;
        m_SwapchainImage = VK_NULL_HANDLE;
        m_SwapchainImageView = VK_NULL_HANDLE;
        m_SwapchainExtent = {};
        m_RenderViewport = {};
        m_RenderOutput = {};
        m_SwapchainImageLayout = nullptr;
        m_DepthTexture = nullptr;
        m_BoundGraphicsPipeline = nullptr;
        m_RenderingActive = false;
    }

    void VulkanCommandList::EndRenderingIfActive()
    {
        if (!m_RenderingActive || m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdEndRendering(m_CommandBuffer);
        m_RenderingActive = false;
    }

    void VulkanCommandList::SetRenderOutput(const RHIRenderOutputDesc& output)
    {
        if (m_RenderingActive)
        {
            XENGINE_LOG_WARN("Render output must be set before the first graphics pipeline is bound");
            return;
        }

        m_RenderOutput = output;
        m_RenderViewport = output.Viewport;
    }

    void VulkanCommandList::SetGraphicsPipeline(RHIPipeline* pipeline)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || pipeline == nullptr)
        {
            return;
        }

        XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
        auto* vulkanPipeline = CheckedVulkanCast<VulkanPipeline>(pipeline, *m_Device);
        if (!vulkanPipeline->IsValid())
        {
            XENGINE_LOG_ERROR("Attempted to bind an invalid Vulkan graphics pipeline");
            return;
        }

        BeginRenderingIfNeeded();
        m_BoundGraphicsPipeline = vulkanPipeline;
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetHandle());
    }

    void VulkanCommandList::SetRenderViewport(const RHIRect2D& viewport)
    {
        if (m_RenderingActive)
        {
            XENGINE_LOG_WARN("Render viewport must be set before the first graphics pipeline is bound");
            return;
        }

        m_RenderViewport = viewport;
        m_RenderOutput.Viewport = viewport;
    }

    void VulkanCommandList::TransitionTextureToShaderRead(RHITexture* texture)
    {
        XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
        auto* vulkanTexture = CheckedVulkanCast<VulkanTexture>(texture, *m_Device);
        if (vulkanTexture == nullptr)
        {
            return;
        }

        // Editor viewport color is written as an attachment, then sampled by
        // the editor UI in the same frame.
        EndRenderingIfActive();
        TransitionColorImage(*vulkanTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanCommandList::SetBindGroup(u32 setIndex, RHIBindGroup* bindGroup)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundGraphicsPipeline == nullptr || bindGroup == nullptr)
        {
            return;
        }

        XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
        auto* vulkanBindGroup = CheckedVulkanCast<VulkanBindGroup>(bindGroup, *m_Device);
        if (vulkanBindGroup == nullptr || vulkanBindGroup->GetHandle() == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Attempted to bind an invalid Vulkan bind group");
            return;
        }

        VkDescriptorSet descriptorSet = vulkanBindGroup->GetHandle();
        vkCmdBindDescriptorSets(
            m_CommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_BoundGraphicsPipeline->GetLayout(),
            setIndex,
            1,
            &descriptorSet,
            0,
            nullptr);
    }

    void VulkanCommandList::SetVertexBuffer(RHIBuffer* buffer, u64 offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || buffer == nullptr)
        {
            return;
        }

        XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
        auto* vulkanBuffer = CheckedVulkanCast<VulkanBuffer>(buffer, *m_Device);
        if (vulkanBuffer == nullptr || !vulkanBuffer->IsValid())
        {
            XENGINE_LOG_ERROR("Attempted to bind an invalid Vulkan vertex buffer");
            return;
        }

        VkBuffer vkBuffer = vulkanBuffer->GetHandle();
        VkDeviceSize vkOffset = offset;
        vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vkBuffer, &vkOffset);
    }

    void VulkanCommandList::SetIndexBuffer(RHIBuffer* buffer, RHIIndexFormat format, u64 offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || buffer == nullptr)
        {
            return;
        }

        XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
        auto* vulkanBuffer = CheckedVulkanCast<VulkanBuffer>(buffer, *m_Device);
        if (vulkanBuffer == nullptr || !vulkanBuffer->IsValid())
        {
            XENGINE_LOG_ERROR("Attempted to bind an invalid Vulkan index buffer");
            return;
        }

        const VkIndexType indexType = format == RHIIndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(m_CommandBuffer, vulkanBuffer->GetHandle(), offset, indexType);
    }

    void VulkanCommandList::PushConstants(RHIShaderStageFlags, const void* data, std::size_t size, std::size_t offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundGraphicsPipeline == nullptr || data == nullptr || size == 0)
        {
            return;
        }

        vkCmdPushConstants(
            m_CommandBuffer,
            m_BoundGraphicsPipeline->GetLayout(),
            m_BoundGraphicsPipeline->GetPushConstantStages(),
            static_cast<u32>(offset),
            static_cast<u32>(size),
            data);
    }

    void VulkanCommandList::Draw(
        u32 vertexCount,
        u32 instanceCount,
        u32 firstVertex,
        u32 firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundGraphicsPipeline == nullptr)
        {
            return;
        }

        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandList::DrawIndexed(
        u32 indexCount,
        u32 instanceCount,
        u32 firstIndex,
        i32 vertexOffset,
        u32 firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundGraphicsPipeline == nullptr)
        {
            return;
        }

        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandList::BeginRenderingIfNeeded()
    {
        if (m_RenderingActive)
        {
            return;
        }

        if (m_CommandBuffer == VK_NULL_HANDLE || m_SwapchainImageLayout == nullptr)
        {
            return;
        }

        VulkanTexture* colorTexture = nullptr;
        VulkanTexture* depthTexture = m_DepthTexture;
        VkImageView colorImageView = m_SwapchainImageView;
        VkImageLayout colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        bool renderToSwapchain = m_RenderOutput.RenderToSwapchain;
        if (!renderToSwapchain)
        {
            XENGINE_ASSERT(m_Device != nullptr, "VulkanCommandList has no owning VulkanDevice");
            colorTexture = CheckedVulkanCast<VulkanTexture>(m_RenderOutput.ColorTarget, *m_Device);
            depthTexture = CheckedVulkanCast<VulkanTexture>(m_RenderOutput.DepthTarget, *m_Device);
            if (colorTexture == nullptr || !colorTexture->IsValid())
            {
                XENGINE_LOG_ERROR("Offscreen render output requires a valid Vulkan color texture");
                return;
            }

            colorImageView = CheckedVulkanCast<VulkanTextureView>(colorTexture->GetDefaultView(), *m_Device)->GetHandle();
        }
        else if (m_SwapchainImage == VK_NULL_HANDLE || m_SwapchainImageView == VK_NULL_HANDLE)
        {
            return;
        }

        if (renderToSwapchain)
        {
            // Sandbox keeps rendering directly to the swapchain; editor switches
            // RenderOutputDesc to an offscreen texture.
            TransitionSwapchainImage(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        else
        {
            TransitionColorImage(*colorTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        if (depthTexture != nullptr)
        {
            TransitionDepthImage(*depthTexture, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        }

        VkRenderingAttachmentInfo colorAttachment {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorImageView;
        colorAttachment.imageLayout = colorLayout;
        colorAttachment.loadOp = renderToSwapchain ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { { 0.1f, 0.1f, 0.15f, 1.0f } };

        VkRenderingAttachmentInfo depthAttachment {};
        if (depthTexture != nullptr)
        {
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = depthTexture->GetImageView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
        }

        RHIRect2D renderViewport = m_RenderViewport;
        const u32 outputWidth = renderToSwapchain ? m_SwapchainExtent.width : m_RenderOutput.Viewport.Width;
        const u32 outputHeight = renderToSwapchain ? m_SwapchainExtent.height : m_RenderOutput.Viewport.Height;
        renderViewport.X = renderToSwapchain ? std::min(renderViewport.X, outputWidth) : 0;
        renderViewport.Y = renderToSwapchain ? std::min(renderViewport.Y, outputHeight) : 0;
        renderViewport.Width = std::min(renderViewport.Width, outputWidth - renderViewport.X);
        renderViewport.Height = std::min(renderViewport.Height, outputHeight - renderViewport.Y);
        if (renderViewport.Width == 0 || renderViewport.Height == 0)
        {
            renderViewport = RHIRect2D { 0, 0, outputWidth, outputHeight };
        }

        VkRenderingInfo renderingInfo {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = {
            static_cast<i32>(renderViewport.X),
            static_cast<i32>(renderViewport.Y)
        };
        renderingInfo.renderArea.extent = { renderViewport.Width, renderViewport.Height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = depthTexture != nullptr ? &depthAttachment : nullptr;

        vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);
        m_RenderingActive = true;

        VkViewport vkViewport {};
        vkViewport.x = static_cast<float>(renderViewport.X);
        vkViewport.y = static_cast<float>(renderViewport.Y);
        vkViewport.width = static_cast<float>(renderViewport.Width);
        vkViewport.height = static_cast<float>(renderViewport.Height);
        vkViewport.minDepth = 0.0f;
        vkViewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &vkViewport);

        VkRect2D scissor {};
        scissor.offset = {
            static_cast<i32>(renderViewport.X),
            static_cast<i32>(renderViewport.Y)
        };
        scissor.extent = { renderViewport.Width, renderViewport.Height };
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
    }

    void VulkanCommandList::TransitionSwapchainImage(VkImageLayout newLayout)
    {
        if (m_SwapchainImageLayout == nullptr || *m_SwapchainImageLayout == newLayout)
        {
            return;
        }

        VkImageSubresourceRange range {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = GetSourceAccess(*m_SwapchainImageLayout);
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = *m_SwapchainImageLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_SwapchainImage;
        barrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            m_CommandBuffer,
            GetSourceStage(*m_SwapchainImageLayout),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        *m_SwapchainImageLayout = newLayout;
    }

    void VulkanCommandList::TransitionColorImage(VulkanTexture& texture, VkImageLayout newLayout)
    {
        VkImageLayout* layout = texture.GetLayoutPtr();
        if (layout == nullptr || *layout == newLayout)
        {
            return;
        }

        VkAccessFlags dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            dstAccess = VK_ACCESS_SHADER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        VkImageSubresourceRange range {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = GetSourceAccess(*layout);
        barrier.dstAccessMask = dstAccess;
        barrier.oldLayout = *layout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.GetImage();
        barrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            m_CommandBuffer,
            GetSourceStage(*layout),
            dstStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        *layout = newLayout;
    }

    void VulkanCommandList::TransitionDepthImage(VulkanTexture& texture, VkImageLayout newLayout)
    {
        VkImageLayout* layout = texture.GetLayoutPtr();
        if (layout == nullptr || *layout == newLayout)
        {
            return;
        }

        VkImageSubresourceRange range {};
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = *layout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.GetImage();
        barrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            m_CommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        *layout = newLayout;
    }
}
