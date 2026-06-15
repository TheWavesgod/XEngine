#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>

#include <type_traits>

namespace XEngine
{
    static constexpr u32 MaxGPULights = 16;

    enum class GPULightType : u32
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    // Shader interop type.
    // Must stay layout-compatible with Engine/Shaders/Lighting/LightingTypes.slang.
    struct alignas(16) GPULight
    {
        // xyz = world-space position, w = range.
        Vec4 PositionRange { 0.0f };

        // xyz = direction from shaded point to light, w = GPULightType.
        Vec4 DirectionType { 0.0f };

        // rgb = color, w = intensity.
        Vec4 ColorIntensity { 1.0f };

        // x = inner cone angle, y = outer cone angle, z = cast shadow flag, w = padding.
        Vec4 SpotAnglesShadow { 0.0f };
    };

    // Shader interop type.
    // Must stay layout-compatible with Engine/Shaders/Lighting/LightingTypes.slang.
    struct alignas(16) GPULightingData
    {
        // rgb = ambient color, w = ambient intensity.
        Vec4 AmbientColorIntensity { 0.03f, 0.03f, 0.03f, 1.0f };

        // x = light count, yzw = padding.
        Vec4 LightCountAndPadding { 0.0f };

        GPULight Lights[MaxGPULights] {};
    };

    static_assert(sizeof(Vec4) == sizeof(float) * 4);
    static_assert(std::is_standard_layout_v<GPULight>);
    static_assert(std::is_standard_layout_v<GPULightingData>);
    static_assert(sizeof(GPULight) % 16 == 0);
    static_assert(sizeof(GPULightingData) % 16 == 0);
}
