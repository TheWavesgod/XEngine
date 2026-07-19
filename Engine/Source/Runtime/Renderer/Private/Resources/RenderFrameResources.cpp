#include "RenderFrameResources.h"

#include "../Pipeline/RenderFrameContext.h"
#include "../Shadows/RenderShadowManager.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>
#include <XEngine/RHI/RHIUploadManager.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace XEngine
{
    bool RenderFrameResources::Initialize(
        RHIDevice* device,
        RHITextureView* shadowSampledView,
        RHISampler* shadowSampler)
    {
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderFrameResources requires a valid RHIDevice");
            return false;
        }

        m_Device = device;
        m_ShadowSampledView = shadowSampledView;
        m_ShadowSampler = shadowSampler;
        m_HasRealShadow = (shadowSampledView != nullptr && shadowSampler != nullptr);
        if (!m_HasRealShadow)
        {
            CreatePlaceholderShadow(*device);
            m_ShadowSampledView = m_PlaceholderShadowView.get();
            m_ShadowSampler = m_PlaceholderShadowSampler.get();
            XENGINE_LOG_WARN("FrameResources: shadow bind group using placeholder; will rebind when ShadowManager acquires real resources.");
        }
        RHIResourceFactory& factory = m_Device->GetResourceFactory();

        RHIBindGroupLayoutDesc layoutDesc;
        layoutDesc.DebugName = "GPUFrameData bind group layout";
        layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            0, RHIBindingType::UniformBuffer, RHIShaderStageFlags::AllGraphics, 1
        });
        layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry{
            1, RHIBindingType::SampledTexture, RHIShaderStageFlags::Fragment, 1
        });
        layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            2, RHIBindingType::Sampler, RHIShaderStageFlags::Fragment, 1
        });

        m_FrameBindGroupLayout = factory.CreateBindGroupLayout(layoutDesc);
        if (!m_FrameBindGroupLayout)
        {
            XENGINE_LOG_ERROR("Failed to create GPUFrameData bind group layout");
            return false;
        }

        RebuildBindGroups();

        XENGINE_LOG_INFO("RenderFrameResources initialized");
        return true;
    }

    void RenderFrameResources::Shutdown()
    {
        if (m_Device != nullptr)
        {
            XENGINE_LOG_INFO("RenderFrameResources shutdown");
        }

        for (auto& bindGroup : m_FrameBindGroups)
        {
            bindGroup.reset();
        }
        for (auto& buffer : m_FrameBuffers)
        {
            buffer.reset();
        }
        m_FrameBindGroupLayout.reset();
        DestroyPlaceholderShadow();
        m_ShadowSampledView = nullptr;
        m_ShadowSampler = nullptr;
        m_HasRealShadow = false;
        m_Device = nullptr;
    }

    void RenderFrameResources::SetShadowBindings(RHITextureView* shadowSampledView, RHISampler* shadowSampler)
    {
        if (shadowSampledView == nullptr || shadowSampler == nullptr)
        {
            // Keep current bindings if either side is missing.
            return;
        }
        if (shadowSampledView == m_ShadowSampledView && shadowSampler == m_ShadowSampler)
        {
            return;
        }
        m_ShadowSampledView = shadowSampledView;
        m_ShadowSampler = shadowSampler;
        m_HasRealShadow = true;
        const auto viewPtr = reinterpret_cast<std::uintptr_t>(shadowSampledView);
        const auto samplerPtr = reinterpret_cast<std::uintptr_t>(shadowSampler);
        XENGINE_LOG_INFO("SetShadowBindings: switching to real shadow view=0x" + std::to_string(viewPtr)
            + " sampler=0x" + std::to_string(samplerPtr));
        if (m_FrameBindGroupLayout == nullptr || m_Device == nullptr)
        {
            return;
        }
        RebuildBindGroups();
    }

    void RenderFrameResources::CreatePlaceholderShadow(RHIDevice& device)
    {
        RHIResourceFactory& factory = device.GetResourceFactory();

        // 1x1 D32 texture; depth = 0 so reverse-Z compare (`>= 0`) returns 1.0 (lit everywhere).
        RHITextureDesc texDesc {};
        texDesc.Width = 1;
        texDesc.Height = 1;
        texDesc.MipLevels = 1;
        texDesc.ArrayLayers = 1;
        texDesc.Format = RHIFormat::D32Float;
        texDesc.Dimension = RHITextureDimension::Texture2D;
        texDesc.Usage = RHITextureUsageFlags::DepthStencilAttachment
                      | RHITextureUsageFlags::Sampled;
        texDesc.DebugName = "PlaceholderShadowTexture";
        m_PlaceholderShadowTexture = factory.CreateTexture(texDesc);
        if (!m_PlaceholderShadowTexture)
        {
            XENGINE_LOG_ERROR("Failed to create placeholder shadow texture");
            return;
        }

        RHITextureViewDesc sampledDesc {};
        sampledDesc.Texture = m_PlaceholderShadowTexture.get();
        sampledDesc.Usage = RHITextureViewUsageFlags::Sampled;
        sampledDesc.ViewDimension = RHITextureViewDimension::Texture2D;
        sampledDesc.Aspect = RHITextureAspectFlags::Depth;
        sampledDesc.Format = RHIFormat::D32Float;
        sampledDesc.BaseMipLevel = 0;
        sampledDesc.MipCount = 1;
        sampledDesc.BaseArrayLayer = 0;
        sampledDesc.ArrayLayerCount = 1;
        sampledDesc.DebugName = "PlaceholderShadowSampledView";
        m_PlaceholderShadowView = factory.CreateTextureView(sampledDesc);
        if (!m_PlaceholderShadowView)
        {
            XENGINE_LOG_ERROR("Failed to create placeholder shadow view");
            m_PlaceholderShadowTexture.reset();
            return;
        }

        RHISamplerDesc samplerDesc {};
        samplerDesc.MinFilter = RHIFilter::Nearest;
        samplerDesc.MagFilter = RHIFilter::Nearest;
        samplerDesc.AddressU = RHIAddressMode::ClampToEdge;
        samplerDesc.AddressV = RHIAddressMode::ClampToEdge;
        samplerDesc.AddressW = RHIAddressMode::ClampToEdge;
        samplerDesc.MaxAnisotropy = 1.0f;
        samplerDesc.DebugName = "PlaceholderShadowSampler";
        m_PlaceholderShadowSampler = factory.CreateSampler(samplerDesc);
        if (!m_PlaceholderShadowSampler)
        {
            XENGINE_LOG_ERROR("Failed to create placeholder shadow sampler");
            m_PlaceholderShadowView.reset();
            m_PlaceholderShadowTexture.reset();
            return;
        }
    }

    void RenderFrameResources::DestroyPlaceholderShadow()
    {
        m_PlaceholderShadowSampler.reset();
        m_PlaceholderShadowView.reset();
        m_PlaceholderShadowTexture.reset();
    }

    void RenderFrameResources::RebuildBindGroups()
    {
        if (m_Device == nullptr || m_FrameBindGroupLayout == nullptr)
        {
            return;
        }
        RHIResourceFactory& factory = m_Device->GetResourceFactory();

        const GPUFrameData initialData {};
        for (u32 index = 0; index < RendererMaxFramesInFlight; ++index)
        {
            if (!m_FrameBuffers[index])
            {
                RHIBufferDesc bufferDesc;
                bufferDesc.Size = sizeof(GPUFrameData);
                bufferDesc.Usage = RHIBufferUsage::Uniform;
                bufferDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
                bufferDesc.DebugName = "GPUFrameData buffer";
                m_FrameBuffers[index] = factory.CreateBuffer(bufferDesc);
                if (!m_FrameBuffers[index])
                {
                    XENGINE_LOG_ERROR("Failed to create GPUFrameData buffer");
                    continue;
                }
                m_Device->GetUploadManager().UploadBuffer(
                    *m_FrameBuffers[index],
                    &initialData,
                    sizeof(initialData));
            }

            RHIBindGroupDesc bindGroupDesc;
            bindGroupDesc.Layout = m_FrameBindGroupLayout.get();
            bindGroupDesc.DebugName = "GPUFrameData bind group";
            bindGroupDesc.Resources.push_back(RHIBindingResource {
                0,
                RHIBindingType::UniformBuffer,
                nullptr,
                nullptr,
                m_FrameBuffers[index].get()
            });
            bindGroupDesc.Resources.push_back(RHIBindingResource {
                1, RHIBindingType::SampledTexture, m_ShadowSampledView, nullptr, nullptr, 0, 0
            });
            bindGroupDesc.Resources.push_back(RHIBindingResource {
                2, RHIBindingType::Sampler, nullptr, m_ShadowSampler, nullptr, 0, 0
            });

            m_FrameBindGroups[index] = factory.CreateBindGroup(bindGroupDesc);
            if (!m_FrameBindGroups[index])
            {
                XENGINE_LOG_ERROR("Failed to create GPUFrameData bind group");
            }
        }
    }

    void RenderFrameResources::Update(const RenderFrameContext& frame, const RenderScene& scene, const RenderShadowManager& shadowManager)
    {
        RHIBuffer* buffer = GetFrameBuffer(frame.FrameIndex);
        if (buffer == nullptr)
        {
            return;
        }

        const GPUFrameData data = BuildGPUFrameData(frame, scene, shadowManager);
        if (!buffer->Update(&data, sizeof(data)))
        {
            XENGINE_LOG_ERROR("Failed to update GPUFrameData buffer");
        }
    }

    RHIBuffer* RenderFrameResources::GetFrameBuffer(u32 frameIndex) const
    {
        return m_FrameBuffers[GetResourceIndex(frameIndex)].get();
    }

    RHIBindGroup* RenderFrameResources::GetFrameBindGroup(u32 frameIndex) const
    {
        return m_FrameBindGroups[GetResourceIndex(frameIndex)].get();
    }

    RHIBindGroupLayout* RenderFrameResources::GetFrameBindGroupLayout() const
    {
        return m_FrameBindGroupLayout.get();
    }

    GPUFrameData RenderFrameResources::BuildGPUFrameData(
        const RenderFrameContext& frame,
        const RenderScene& scene,
        const RenderShadowManager& shadowManager) const
    {
        GPUFrameData data {};
        data.Camera.View = frame.ViewMatrix;
        data.Camera.Projection = frame.ProjectionMatrix;
        data.Camera.ViewProjection = frame.ViewProjectionMatrix;
        data.Camera.CameraPosition = Vec4(frame.CameraWorldPosition, 0.0f);
        data.Lighting = BuildGPULightingData(scene);
        // Fill per-frame shadow uniforms (cascade matrices, biases, params).
        // Without this, shader-side Cascades[] is always zero and shadow sampling collapses.
        shadowManager.FillGPUShadowData(data.Shadows);
        return data;
    }

    GPULightingData RenderFrameResources::BuildGPULightingData(const RenderScene& scene) const
    {
        GPULightingData data {};
        data.AmbientColorIntensity = Vec4 { 0.03f, 0.03f, 0.03f, 1.0f };

        u32 packedLightCount = 0;
        const u32 lightLimit = std::min<u32>(static_cast<u32>(scene.Lights.size()), MaxGPULights);
        for (u32 lightIndex = 0; lightIndex < lightLimit; ++lightIndex)
        {
            const RenderLight& renderLight = scene.Lights[lightIndex];
            if (!renderLight.Enabled)
            {
                continue;
            }

            GPULight& gpuLight = data.Lights[packedLightCount];
            const bool castsShadow = renderLight.CastShadow;

            switch (renderLight.Type)
            {
            case RenderLightType::Directional:
                gpuLight.PositionRange = Vec4 { 0.0f, 0.0f, 0.0f, 0.0f };

                // XEngine convention:
                // Light forward is rotated +X and represents the direction light rays travel.
                // Shading uses direction from surface point to light, so DirectionToLight is -forward.
                gpuLight.DirectionType = Vec4 {
                    Math::Normalize(renderLight.DirectionToLight),
                    static_cast<float>(GPULightType::Directional)
                };
                gpuLight.ColorIntensity = Vec4 { renderLight.Color, renderLight.Intensity };
                gpuLight.SpotAnglesShadow = Vec4 { 0.0f, 0.0f, castsShadow ? 1.0f : 0.0f, 0.0f };
                break;
            case RenderLightType::Point:
                // Stage 8C evaluates directional lights first.
                // Point and spot fields are packed so the layout can grow naturally later.
                gpuLight.PositionRange = Vec4 { renderLight.Position, renderLight.Range };
                gpuLight.DirectionType = Vec4 { 0.0f, 0.0f, 0.0f, static_cast<float>(GPULightType::Point) };
                gpuLight.ColorIntensity = Vec4 { renderLight.Color, renderLight.Intensity };
                gpuLight.SpotAnglesShadow = Vec4 { 0.0f, 0.0f, castsShadow ? 1.0f : 0.0f, 0.0f };
                break;
            case RenderLightType::Spot:
                gpuLight.PositionRange = Vec4 { renderLight.Position, renderLight.Range };
                gpuLight.DirectionType = Vec4 {
                    Math::Normalize(renderLight.DirectionToLight),
                    static_cast<float>(GPULightType::Spot)
                };
                gpuLight.ColorIntensity = Vec4 { renderLight.Color, renderLight.Intensity };
                gpuLight.SpotAnglesShadow = Vec4 {
                    renderLight.InnerConeAngleRadians,
                    renderLight.OuterConeAngleRadians,
                    castsShadow ? 1.0f : 0.0f,
                    0.0f
                };
                break;
            }

            ++packedLightCount;
            if (packedLightCount >= MaxGPULights)
            {
                break;
            }
        }

        data.LightCountAndPadding = Vec4 { static_cast<float>(packedLightCount), 0.0f, 0.0f, 0.0f };
        return data;
    }

    u32 RenderFrameResources::GetResourceIndex(u32 frameIndex) const
    {
        return frameIndex % RendererMaxFramesInFlight;
    }
}
