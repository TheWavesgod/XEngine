#include "EditorViewportRenderTarget.h"

#include <XEngine/RHI/RHIDevice.h>

#include <algorithm>

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

        m_Width = width;
        m_Height = height;

        RHITextureDesc colorDesc;
        colorDesc.Width = m_Width;
        colorDesc.Height = m_Height;
        colorDesc.Format = RHIFormat::BGRA8Unorm;
        // The editor viewport color target is rendered into, then sampled by
        // ImGui when drawing the Viewport panel.
        colorDesc.Usage = RHITextureUsageFlags::ColorAttachment | RHITextureUsageFlags::Sampled;
        colorDesc.DebugName = "Editor viewport color";
        m_ColorTexture = device.CreateTexture(colorDesc, nullptr, 0);

        RHITextureDesc depthDesc;
        depthDesc.Width = m_Width;
        depthDesc.Height = m_Height;
        depthDesc.Format = RHIFormat::D32Float;
        depthDesc.Usage = RHITextureUsageFlags::DepthStencilAttachment;
        depthDesc.DebugName = "Editor viewport depth";
        m_DepthTexture = device.CreateTexture(depthDesc, nullptr, 0);

        if (m_Sampler == nullptr)
        {
            RHISamplerDesc samplerDesc;
            samplerDesc.AddressU = RHIAddressMode::ClampToEdge;
            samplerDesc.AddressV = RHIAddressMode::ClampToEdge;
            samplerDesc.AddressW = RHIAddressMode::ClampToEdge;
            samplerDesc.DebugName = "Editor viewport sampler";
            m_Sampler = device.CreateSampler(samplerDesc);
        }

        return IsValid() && m_Sampler != nullptr;
    }

    void EditorViewportRenderTarget::Reset()
    {
        m_ColorTexture.reset();
        m_DepthTexture.reset();
        m_Sampler.reset();
        m_Width = 0;
        m_Height = 0;
    }

    RHIRenderOutputDesc EditorViewportRenderTarget::BuildRenderOutput() const
    {
        RHIRenderOutputDesc output;
        output.ColorTarget = m_ColorTexture.get();
        output.DepthTarget = m_DepthTexture.get();
        output.Viewport = RHIRect2D { 0, 0, m_Width, m_Height };
        output.ColorFormat = RHIFormat::BGRA8Unorm;
        output.DepthFormat = RHIFormat::D32Float;
        output.RenderToSwapchain = false;
        return output;
    }
}
