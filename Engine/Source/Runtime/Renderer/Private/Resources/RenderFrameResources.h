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
    class RHITexture;
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

        // Rebind Set 0 binding 1/2 (sampled texture / sampler) to freshly-built shadow
        // resources. Used when the shadow cache is recreated at runtime. Caller
        // must guarantee the new pointers remain valid until the next SetShadowBindings.
        void SetShadowBindings(RHITextureView* shadowSampledView, RHISampler* shadowSampler);

        RHIBuffer* GetFrameBuffer(u32 frameIndex) const;
        RHIBindGroup* GetFrameBindGroup(u32 frameIndex) const;
        RHIBindGroupLayout* GetFrameBindGroupLayout() const;

    private:
        GPUFrameData BuildGPUFrameData(const RenderFrameContext& frame, const RenderScene& scene, const RenderShadowManager& shadowManager) const;
        GPULightingData BuildGPULightingData(const RenderScene& scene) const;
        u32 GetResourceIndex(u32 frameIndex) const;

        // Build a placeholder 1x1 D32 shadow texture + nearest-clamp sampler so
        // Set 0 binding 1/2 is never null. The placeholder reads shadow depth = 0,
        // so a reverse-Z comparison with `>=` returns 1.0 (full lit). This matches
        // the behaviour we want before the real shadow cascade is allocated.
        void CreatePlaceholderShadow(RHIDevice& device);
        void DestroyPlaceholderShadow();

        // Rebuild m_FrameBindGroups using current shadow bindings. Used whenever
        // the shadow resource identity changes (initial creation, runtime swap).
        void RebuildBindGroups();

    private:
        RHIDevice* m_Device = nullptr;
        std::shared_ptr<RHIBindGroupLayout> m_FrameBindGroupLayout;

        std::array<std::shared_ptr<RHIBuffer>, RendererMaxFramesInFlight> m_FrameBuffers;
        std::array<std::shared_ptr<RHIBindGroup>, RendererMaxFramesInFlight> m_FrameBindGroups;

        // Captured at Initialize() so Set 0 binding 1/2 (sampled texture / sampler)
        // references the same persistent shadow resources across frames.
        // May point at m_Placeholder* until SetShadowBindings replaces it.
        RHITextureView* m_ShadowSampledView = nullptr;
        RHISampler* m_ShadowSampler = nullptr;

        // Owned fallback resources used only when the application has not yet
        // produced a real shadow array. Created lazily inside CreatePlaceholderShadow().
        std::shared_ptr<RHITexture> m_PlaceholderShadowTexture;
        std::shared_ptr<RHITextureView> m_PlaceholderShadowView;
        std::shared_ptr<RHISampler> m_PlaceholderShadowSampler;

        bool m_HasRealShadow = false;
    };
}
