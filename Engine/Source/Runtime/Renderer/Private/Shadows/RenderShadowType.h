#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Math.h>

#include <array>

namespace XEngine 
{
    static constexpr u32 MaxShadowCascades = 4;

    class RHITexture;
    class RHITextureView;
    class RHISampler;

    struct RenderShadowCascade
    {
        Mat4 LightView = Mat4(1.0f);
        Mat4 LightProjection = Mat4(1.0f);
        Mat4 LightViewProjection = Mat4(1.0f);

        float SplitNear = 0.0f;
        float SplitFar = 0.0f;

        u32 LayerIndex = 0;
        u32 Resolution = 0;

        Vec4 ShadowMapSize = Vec4(0.0f);
        
        // x = depth bias   y = normal bias   z = slope-scaled bias factor   w = reserved
        Vec4 BiasParams = Vec4(0.0f);

        AABB WorldBounds;
        AABB LightSpaceBounds;
    };

    struct RenderDirectionalShadowFrameData
    {
        bool Enabled = false;
        u32 CascadeCount = 0;

        RHITexture* ShadowTexture = nullptr;

        // Sampled view of the whole texture array.
        RHITextureView* SampledView = nullptr;

        // Per-cascade depth attachment views.
        std::array<RHITextureView*, MaxShadowCascades> CascadeDepthViews {};

        RHISampler* Sampler = nullptr;

        std::array<RenderShadowCascade, MaxShadowCascades> Cascades;
    };

    struct RenderShadowFrameData
    {
        RenderDirectionalShadowFrameData Directional;
    };
}