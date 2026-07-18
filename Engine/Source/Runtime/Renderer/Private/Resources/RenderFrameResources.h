#pragma once

#include "../ShaderInterop/GPUFrameTypes.h"

#include <XEngine/Core/Types.h>

#include <array>
#include <memory>

namespace XEngine
{
    class RHIBindGroup;
    class RHIBindGroupLayout;
    class RHIBuffer;
    class RHIDevice;
    class RHISampler;
    class RHITextureView;
    struct RenderFrameContext;
    struct RenderScene;
    class RenderShadowManager;

    static constexpr u32 RendererMaxFramesInFlight = 3;

    // Per-frame shader-visible renderer data.
    // Set 0 in forward shaders is expected to bind this data.
    class RenderFrameResources
    {
    public:
        bool Initialize(
            RHIDevice* device,
            RHITextureView* shadowSampledView = nullptr,
            RHISampler* shadowSampler = nullptr);
        void Shutdown();

        void Update(const RenderFrameContext& frame, const RenderScene& scene, const RenderShadowManager& shadowManager);

        RHIBuffer* GetFrameBuffer(u32 frameIndex) const;
        RHIBindGroup* GetFrameBindGroup(u32 frameIndex) const;
        RHIBindGroupLayout* GetFrameBindGroupLayout() const;

    private:
        GPUFrameData BuildGPUFrameData(const RenderFrameContext& frame, const RenderScene& scene, const RenderShadowManager& shadowManager) const;
        GPULightingData BuildGPULightingData(const RenderScene& scene) const;
        u32 GetResourceIndex(u32 frameIndex) const;

    private:
        RHIDevice* m_Device = nullptr;
        std::shared_ptr<RHIBindGroupLayout> m_FrameBindGroupLayout;

        std::array<std::shared_ptr<RHIBuffer>, RendererMaxFramesInFlight> m_FrameBuffers;
        std::array<std::shared_ptr<RHIBindGroup>, RendererMaxFramesInFlight> m_FrameBindGroups;

        // Captured at Initialize() so Set 0 binding 1/2 (sampled texture / sampler)
        // references the same persistent shadow resources across frames.
        RHITextureView* m_ShadowSampledView = nullptr;
        RHISampler* m_ShadowSampler = nullptr;
    };
}
