#include "EditorViewportRenderTarget.h"

#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>

#include <algorithm>
#include <utility>

namespace XEngine
{
    bool EditorViewportRenderTarget::Resize(RHIDevice& device, u32 width, u32 height)
    {
        width = std::max(1u, width);
        height = std::max(1u, height);
        if (m_ColorTexture != nullptr && m_DepthTexture != nullptr &&
            m_Width == width && m_Height == height)
        {
            return true;
        }

        RHIResourceFactory& factory = device.GetResourceFactory();

        RHITextureDesc colorDesc;
        colorDesc.Width = width;
        colorDesc.Height = height;
        colorDesc.Format = RHIFormat::BGRA8Unorm;
        // The editor viewport color target is rendered into, then sampled by
        // ImGui when drawing the Viewport panel.
        colorDesc.Usage = RHITextureUsageFlags::ColorAttachment | RHITextureUsageFlags::Sampled;
        colorDesc.DebugName = "Editor viewport color";
        std::shared_ptr<RHITexture> colorTexture = factory.CreateTexture(colorDesc);
        if (!colorTexture)
        {
            return false;
        }

        RHITextureViewDesc colorViewDesc;
        colorViewDesc.Texture = colorTexture.get();
        colorViewDesc.Usage =
            RHITextureViewUsageFlags::ColorAttachment |
            RHITextureViewUsageFlags::Sampled;
        colorViewDesc.ViewDimension = RHITextureViewDimension::Texture2D;
        colorViewDesc.Aspect = RHITextureAspectFlags::Color;
        colorViewDesc.Format = colorDesc.Format;
        colorViewDesc.DebugName = "Editor viewport color view";
        std::shared_ptr<RHITextureView> colorView =
            factory.CreateTextureView(colorViewDesc);
        if (!colorView)
        {
            return false;
        }

        RHITextureDesc depthDesc;
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.Format = RHIFormat::D32Float;
        depthDesc.Usage = RHITextureUsageFlags::DepthStencilAttachment;
        depthDesc.DebugName = "Editor viewport depth";
        std::shared_ptr<RHITexture> depthTexture = factory.CreateTexture(depthDesc);
        if (!depthTexture)
        {
            return false;
        }

        RHITextureViewDesc depthViewDesc;
        depthViewDesc.Texture = depthTexture.get();
        depthViewDesc.Usage = RHITextureViewUsageFlags::DepthAttachment;
        depthViewDesc.ViewDimension = RHITextureViewDimension::Texture2D;
        depthViewDesc.Aspect = RHITextureAspectFlags::Depth;
        depthViewDesc.Format = depthDesc.Format;
        depthViewDesc.DebugName = "Editor viewport depth view";
        std::shared_ptr<RHITextureView> depthView =
            factory.CreateTextureView(depthViewDesc);
        if (!depthView)
        {
            return false;
        }

        if (m_Sampler == nullptr)
        {
            RHISamplerDesc samplerDesc;
            samplerDesc.AddressU = RHIAddressMode::ClampToEdge;
            samplerDesc.AddressV = RHIAddressMode::ClampToEdge;
            samplerDesc.AddressW = RHIAddressMode::ClampToEdge;
            samplerDesc.DebugName = "Editor viewport sampler";
            m_Sampler = factory.CreateSampler(samplerDesc);
        }

        if (!m_Sampler)
        {
            return false;
        }

        m_ColorTexture = std::move(colorTexture);
        m_ColorTextureView = std::move(colorView);
        m_DepthTexture = std::move(depthTexture);
        m_DepthTextureView = std::move(depthView);
        m_Width = width;
        m_Height = height;
        return true;
    }

    void EditorViewportRenderTarget::Reset()
    {
        m_ColorTextureView.reset();
        m_DepthTextureView.reset();
        m_ColorTexture.reset();
        m_DepthTexture.reset();
        m_Sampler.reset();
        m_Width = 0;
        m_Height = 0;
    }

    RHIRenderOutputDesc EditorViewportRenderTarget::BuildRenderOutput() const
    {
        RHIRenderOutputDesc output;
        output.ColorTargetView = m_ColorTextureView.get();
        output.DepthTargetView = m_DepthTextureView.get();
        output.Viewport = RHIRect2D { 0, 0, m_Width, m_Height };
        output.ColorFormat = RHIFormat::BGRA8Unorm;
        output.DepthFormat = RHIFormat::D32Float;
        output.RenderToSwapchain = false;
        return output;
    }
}
