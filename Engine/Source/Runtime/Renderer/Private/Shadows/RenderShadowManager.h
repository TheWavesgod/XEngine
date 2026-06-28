#pragma once

#include "RenderShadowType.h"
#include "DirectionalShadowPlanner.h"
#include "ShadowResourceCache.h"

namespace XEngine
{
    class RHIDevice;
    class RenderScene;
    struct RenderFrameContext;
    struct ShadowSettings;
    struct ShadowDebugSettings;
    struct GPUShadowData;
    
/*
 * Responsibilities:
    1. Find the first enabled directional light with CastShadow.
    2. Ask ShadowResourceCache for texture array, views, and sampler.
    3. Ask DirectionalShadowPlanner to build cascade data.
    4. Fill RenderShadowFrameData.
    5. Fill GPUShadowData.
    6. Support FreezeShadowMatrices debug mode.
 */
    class RenderShadowManager 
    {
    public: 
        void Initialize(RHIDevice& device); 
        void Shutdown(RHIDevice& device);

        void PrepareFrame(RHIDevice& device, const RenderScene& scene, 
            const RenderFrameContext& frame, const ShadowSettings& settings, 
            const ShadowDebugSettings& debugSettings);

        void FillGPUShadowData(GPUShadowData& outData) const;
        const RenderShadowFrameData& GetFrameData() const;
        bool HasDirectionalShadow() const;

    private: 
        void PrepareDirectionalShadow(RHIDevice& device, const RenderScene& scene, 
            const RenderFrameContext& frame, const DirectionalShadowSettings& settings, 
            const ShadowDebugSettings& debugSettings); 
    
    private: 
        // Per-frame shadow state. Recomputed every frame unless FreezeShadowMatrices is on.
        RenderShadowFrameData m_FrameData; 

        // Pure math; stateless. Owned by value.
        DirectionalShadowPlanner m_DirectionalPlanner; 

        // Owns shadow GPU resources (texture array, sampled view, per-layer depth views, sampler).
        ShadowResourceCache m_ResourceCache; 

        // True after the user enables FreezeShadowMatrices and we have captured one frame of data.
        bool m_HasFrozenData = false; 

        // Snapshot of m_FrameData taken when FreezeShadowMatrices was enabled. 
        // Used to keep matrices stable across frames while the toggle is on.
        RenderShadowFrameData m_FrozenFrameData;   
    };
}