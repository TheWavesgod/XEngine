#include "ShadowResourceCache.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>

#include <string>

// For Stage 9 V0, this can manage only one directional shadow texture array.
// Future expansion can add atlas, spot shadow, point shadow, VSM/EVSM textures, and blur temporaries.

namespace XEngine
{
    void ShadowResourceCache::Initialize(RHIDevice& device)
    {
        XENGINE_ASSERT(device.IsValid(), "ShadowResourceCache requires a valid RHIDevice");
        m_Device = &device;
        // Nothing to do up front. Resources are created lazily on first
        // GetOrCreateDirectionalShadowResources call.
    }

    void ShadowResourceCache::Shutdown()
    {
        m_Directional = {};
    }

    DirectionalShadowResources& ShadowResourceCache::GetOrCreateDirectionalShadowResources( 
        const DirectionalShadowResourceDesc& desc)
    {
        XENGINE_ASSERT(m_Device != nullptr, "ShadowResourceCache device ref is unvalid");

        // Reject anything other than Texture2DArray in V0.
        if (desc.StorageMode != ShadowMapStorageMode::Texture2DArray)
        {
            XENGINE_LOG_ERROR("Stage 9 V0 only supports ShadowMapStorageMode::Texture2DArray");
            return m_Directional; // empty; caller should treat as failure
        }

        // Validate desc.
        if (desc.CascadeCount == 0 || desc.CascadeCount > MaxShadowCascades)
        {
            XENGINE_LOG_ERROR("Cascade count out of range");
            return m_Directional;
        }
        if (desc.Resolution == 0)
        {
            XENGINE_LOG_ERROR("Shadow resolution must be > 0");
            return m_Directional;
        }
        if (desc.DepthFormat != RHIFormat::D32Float)
        {
            XENGINE_LOG_ERROR("Stage 9 V0 only supports D32Float shadow formats");
            return m_Directional;
        }

        // Match against current slot. Same shape? return existing.
        if (m_Directional.Texture != nullptr
            && m_Directional.CascadeCount == desc.CascadeCount
            && m_Directional.Resolution   == desc.Resolution
            && m_Directional.Format       == desc.DepthFormat)
        {
            return m_Directional;
        }

        // Recreate. Release existing first.
        m_Directional = {};

        m_Directional.CascadeCount = desc.CascadeCount;
        m_Directional.Resolution   = desc.Resolution;
        m_Directional.Format       = desc.DepthFormat;

        auto factory = m_Device->GetResourceFactory();

        // Request whole shadow texture (2d array depth).
        RHITextureDesc texDesc {};
        texDesc.Width        = desc.Resolution;
        texDesc.Height       = desc.Resolution;
        texDesc.MipLevels    = 1;
        texDesc.ArrayLayers  = desc.CascadeCount;
        texDesc.Format       = desc.DepthFormat;
        texDesc.Dimension    = RHITextureDimension::Texture2DArray;
        texDesc.Usage        = RHITextureUsageFlags::DepthStencilAttachment
                                | RHITextureUsageFlags::Sampled;
        texDesc.DebugName    = "DirectionalCascadeShadowArray";
        m_Directional.Texture = factory.CreateTexture(texDesc);
        if (!m_Directional.Texture)
        {
            XENGINE_LOG_ERROR("Failed to create directional shadow texture array");
            m_Directional = {};
            return m_Directional;
        }

        // Whole-array sampled view (used by shader).
        RHITextureViewDesc sampledDesc {};
        sampledDesc.Texture        = m_Directional.Texture.get();
        sampledDesc.Usage          = RHITextureViewUsageFlags::Sampled;
        sampledDesc.ViewDimension  = RHITextureViewDimension::Texture2DArray;
        sampledDesc.Aspect         = RHITextureAspectFlags::Depth;
        sampledDesc.Format         = desc.DepthFormat;
        sampledDesc.BaseMipLevel   = 0;
        sampledDesc.MipCount       = 1;
        sampledDesc.BaseArrayLayer = 0;
        sampledDesc.ArrayLayerCount = 0; // all layers
        sampledDesc.DebugName      = "DirectionalShadowMapArraySampled";
        m_Directional.SampledView  = factory.CreateTextureView(sampledDesc);
        if (!m_Directional.SampledView)
        {
            XENGINE_LOG_ERROR("Failed to create directional shadow sampled view");
            m_Directional = {};
            return m_Directional;
        }

        // Per-layer depth attachment views.
        for (u32 layer = 0; layer < desc.CascadeCount; ++layer)
        {
            std::string debugName = "DirectionalShadowLayerView " + std::to_string(layer);

            RHITextureViewDesc layerDesc {};
            layerDesc.Texture         = m_Directional.Texture.get();
            layerDesc.Usage           = RHITextureViewUsageFlags::DepthAttachment;
            layerDesc.ViewDimension   = RHITextureViewDimension::Texture2DArray;
            layerDesc.Aspect          = RHITextureAspectFlags::Depth;
            layerDesc.Format          = desc.DepthFormat;
            layerDesc.BaseMipLevel    = 0;
            layerDesc.MipCount        = 1;
            layerDesc.BaseArrayLayer  = layer;
            layerDesc.ArrayLayerCount = 1; // single layer
            layerDesc.DebugName       = debugName.data();

            m_Directional.LayerDepthViews[layer] = factory.CreateTextureView(layerDesc);
            if (!m_Directional.LayerDepthViews[layer])
            {
                XENGINE_LOG_ERROR("Failed to create directional shadow per-layer depth view");
                m_Directional = {};
                return m_Directional;
            }
        }

        // Comparison sampler.
        RHISamplerDesc samplerDesc {};
        samplerDesc.MinFilter    = RHIFilter::Linear;
        samplerDesc.MagFilter    = RHIFilter::Linear;
        samplerDesc.AddressU     = RHIAddressMode::ClampToBorder;
        samplerDesc.AddressV     = RHIAddressMode::ClampToBorder;
        samplerDesc.AddressW     = RHIAddressMode::ClampToBorder;
        samplerDesc.MaxAnisotropy = 1.0f;
        samplerDesc.DebugName    = "DirectionalShadowSampler";
        m_Directional.Sampler    = factory.CreateSampler(samplerDesc);
        if (!m_Directional.Sampler)
        {
            XENGINE_LOG_ERROR("Failed to create directional shadow comparison sampler");
            m_Directional = {};
            return m_Directional;
        }
        // TODO: The current RHISamplerDesc does not have a CompareOp or CompareEnable field. 
        // The current Vulkan backend's VulkanSampler hardcodes 
        // compareEnable = VK_FALSE and compareOp = VK_COMPARE_OP_ALWAYS. 
        // For V0 this means we cannot use SampleCmp with hardware PCF.
        // TODO: current VulkanSampler hardcodes borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
        // For shadow sampling with PCF in the shader, you want the border to read as "lit" (1.0 in the comparison)
        // which means the border depth should be 1.0.

        return m_Directional;
    }

}