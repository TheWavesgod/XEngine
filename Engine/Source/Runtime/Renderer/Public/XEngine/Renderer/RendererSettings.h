#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class DirectionalShadowTechnique : u8
    {
        None,
        CascadedShadowMaps
    };

    enum class ShadowMapStorageMode : u8
    {
        Texture2DArray,
        Atlas // future
    };

    enum class ShadowFilterMode : u8 
    { 
        Hard, 
        PCF3x3, 
        PCF5x5, 
        PCSS // future 
    };

    struct DirectionalShadowSettings
    {
        bool Enabled = true;

        DirectionalShadowTechnique Technique = DirectionalShadowTechnique::CascadedShadowMaps;
        ShadowMapStorageMode StorageMode = ShadowMapStorageMode::Texture2DArray;
        ShadowFilterMode FilterMode = ShadowFilterMode::PCF3x3;

        u32 CascadeCount = 4;
        u32 Resolution = 2048;

        float SplitLambda = 0.5f;

        // TODO Stage 9 V0: tune depth bias. 0.003 was overlapping with self-shadow
        // under the reverse-Z `clip.z * 0.5 + 0.5` mapping we ended up with, so
        // default is now 0 to avoid masking shadow acne that needs separate handling.
        float DepthBias = 0.0f;
        float NormalBias = 0.0f;

        bool StabilizeCascades = true;
    };

    struct ShadowSettings
    {
        DirectionalShadowSettings Directional;
    };

    struct RendererSettings 
    { 
        ShadowSettings Shadows; 
    };

} // namespace XEngine
