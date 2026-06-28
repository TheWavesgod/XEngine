#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Math.h>

namespace XEngine
{
    struct RenderLight;
    struct RenderDirectionalShadowFrameData;

    struct DirectionalShadowPlanDesc 
    { 
        const RenderLight* Light = nullptr; 

        Mat4 CameraView         = Mat4(1.0f); 
        Mat4 CameraProjection   = Mat4(1.0f); 
        float CameraNear        = 0.1f; 
        float CameraFar         = 1000.0f; 
        Vec3 CameraPosition     = Vec3(0.0f); 
        
        // Aggregate world-space AABB of all shadow-casting OpaqueObjects.
        // RenderShadowManager builds this by CombineAABB'ing every
        // RenderObject.WorldBounds where object.CastShadow == true.
        AABB SceneBounds; 
        
        u32 CascadeCount    = 4; 
        u32 Resolution      = 2048; 

        float SplitLambda   = 0.5f; 
        float DepthBias     = 0.003f; 
        float NormalBias    = 0.0f; 
        
        bool StabilizeCascades = true; 
        bool ReverseZ       = true;
    };

/*
 * This class owns the CSM math only.
 */
    class DirectionalShadowPlanner
    {
    public:
        bool BuildPlan(const DirectionalShadowPlanDesc& desc, RenderDirectionalShadowFrameData& outData) const;
    };
}