#pragma once

#include <XEngine/RHI/RHICommandList.h>

#include <volk.h>

namespace XEngine
{
    class VulkanBindGroup;
    class VulkanPipeline;
    class VulkanTexture;

    class VulkanCommandList final : public RHICommandList
    {
    public:
        VulkanCommandList() = default;
        ~VulkanCommandList() override = default;

        void BeginFrame(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkImageView swapchainImageView,
            VkExtent2D swapchainExtent,
            VkImageLayout* swapchainImageLayout,
            VulkanTexture* depthTexture);

        void Reset();
        void EndRenderingIfActive();

        void SetRenderOutput(const RHIRenderOutputDesc& output) override;
        void SetGraphicsPipeline(RHIPipeline* pipeline) override;
        void SetRenderViewport(const RHIRect2D& viewport) override;
        void TransitionTextureToShaderRead(RHITexture* texture) override;
        void SetBindGroup(u32 setIndex, RHIBindGroup* bindGroup) override;
        void SetVertexBuffer(RHIBuffer* buffer, u64 offset = 0) override;
        void SetIndexBuffer(RHIBuffer* buffer, RHIIndexFormat format, u64 offset = 0) override;
        void PushConstants(
            RHIShaderStageFlags stages,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) override;

        void Draw(
            u32 vertexCount,
            u32 instanceCount,
            u32 firstVertex,
            u32 firstInstance) override;
        void DrawIndexed(
            u32 indexCount,
            u32 instanceCount,
            u32 firstIndex,
            i32 vertexOffset,
            u32 firstInstance) override;

    private:
        void BeginRenderingIfNeeded();
        void TransitionSwapchainImage(VkImageLayout newLayout);
        void TransitionColorImage(VulkanTexture& texture, VkImageLayout newLayout);
        void TransitionDepthImage(VulkanTexture& texture, VkImageLayout newLayout);

        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkImage m_SwapchainImage = VK_NULL_HANDLE;
        VkImageView m_SwapchainImageView = VK_NULL_HANDLE;
        VkExtent2D m_SwapchainExtent {};
        RHIRect2D m_RenderViewport {};
        RHIRenderOutputDesc m_RenderOutput {};
        VkImageLayout* m_SwapchainImageLayout = nullptr;
        VulkanTexture* m_DepthTexture = nullptr;
        VulkanPipeline* m_BoundGraphicsPipeline = nullptr;
        bool m_RenderingActive = false;
    };
}
