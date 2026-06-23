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
        RenderShadowFrameData m_FrameData; 
        DirectionalShadowPlanner m_DirectionalPlanner; 
        ShadowResourceCache m_ResourceCache; 

        bool m_HasFrozenData = false; 
        RenderShadowFrameData m_FrozenFrameData;   
    };
}