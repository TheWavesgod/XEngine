#include "RenderFrameResources.h"

#include "../Pipeline/RenderFrameContext.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>
#include <XEngine/RHI/RHIUploadManager.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>

#include <algorithm>

namespace XEngine
{
    bool RenderFrameResources::Initialize(RHIDevice* device)
    {
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderFrameResources requires a valid RHIDevice");
            return false;
        }

        m_Device = device;
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

        const GPUFrameData initialData {};
        for (u32 index = 0; index < RendererMaxFramesInFlight; ++index)
        {
            RHIBufferDesc bufferDesc;
            bufferDesc.Size = sizeof(GPUFrameData);
            bufferDesc.Usage = RHIBufferUsage::Uniform;
            bufferDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
            bufferDesc.DebugName = "GPUFrameData buffer";

            // One GPUFrameData buffer per frame-in-flight to avoid overwriting data
            // that may still be used by the GPU.
            m_FrameBuffers[index] = factory.CreateBuffer(bufferDesc);
            if (!m_FrameBuffers[index])
            {
                XENGINE_LOG_ERROR("Failed to create GPUFrameData buffer");
                return false;
            }
            m_Device->GetUploadManager().UploadBuffer(
                *m_FrameBuffers[index],
                &initialData,
                sizeof(initialData));

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
                1, RHIBindingType::SampledTexture, shadowSampledView, nullptr, nullptr, 0, 0
            });
            bindGroupDesc.Resources.push_back(RHIBindingResource {
                2, RHIBindingType::Sampler, nullptr, shadowSampler, nullptr, 0, 0
            }); // TODO: sure the shadow resource descriptor should be handled here? 


            m_FrameBindGroups[index] = factory.CreateBindGroup(bindGroupDesc);
            if (!m_FrameBindGroups[index])
            {
                XENGINE_LOG_ERROR("Failed to create GPUFrameData bind group");
                return false;
            }
        }

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
        m_Device = nullptr;
    }

    void RenderFrameResources::Update(const RenderFrameContext& frame, const RenderScene& scene)
    {
        RHIBuffer* buffer = GetFrameBuffer(frame.FrameIndex);
        if (buffer == nullptr)
        {
            return;
        }

        const GPUFrameData data = BuildGPUFrameData(frame, scene);
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
        const RenderScene& scene) const
    {
        GPUFrameData data {};
        data.Camera.View = frame.ViewMatrix;
        data.Camera.Projection = frame.ProjectionMatrix;
        data.Camera.ViewProjection = frame.ViewProjectionMatrix;
        data.Camera.CameraPosition = Vec4(frame.CameraWorldPosition, 0.0f);
        data.Lighting = BuildGPULightingData(scene);
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
