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
    struct RenderFrameContext;
    struct RenderScene;

    static constexpr u32 RendererMaxFramesInFlight = 3;

    // Per-frame shader-visible renderer data.
    // Set 0 in forward shaders is expected to bind this data.
    class RenderFrameResources
    {
    public:
        bool Initialize(RHIDevice* device);
        void Shutdown();

        void Update(const RenderFrameContext& frame, const RenderScene& scene);

        RHIBuffer* GetFrameBuffer(u32 frameIndex) const;
        RHIBindGroup* GetFrameBindGroup(u32 frameIndex) const;
        RHIBindGroupLayout* GetFrameBindGroupLayout() const;

    private:
        GPUFrameData BuildGPUFrameData(const RenderFrameContext& frame, const RenderScene& scene) const;
        GPULightingData BuildGPULightingData(const RenderScene& scene) const;
        u32 GetResourceIndex(u32 frameIndex) const;

    private:
        RHIDevice* m_Device = nullptr;
        std::shared_ptr<RHIBindGroupLayout> m_FrameBindGroupLayout;

        std::array<std::shared_ptr<RHIBuffer>, RendererMaxFramesInFlight> m_FrameBuffers;
        std::array<std::shared_ptr<RHIBindGroup>, RendererMaxFramesInFlight> m_FrameBindGroups;
    };
}
