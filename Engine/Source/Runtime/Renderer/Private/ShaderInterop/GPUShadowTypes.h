#pragma once

#include "../Shadows/RenderShadowType.h"

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>

#include <type_traits>

namespace XEngine
{
    struct alignas(16) GPUCascadeShadowData
    {
        Mat4 LightViewProjection = Mat4(1.0f);

        // x = split far in view-space depth
        // y = depth bias
        // z = normal bias
        // w = texel size
        Vec4 Params = Vec4(0.0f);
    };

    struct alignas(16) GPUShadowData
    {
        // x = enabled
        // y = cascade count
        // z = shadow resolution
        // w = visualize cascades
        Vec4 ShadowParams = Vec4(0.0f);

        std::array<GPUCascadeShadowData, MaxShadowCascades> Cascades;
    };

    static_assert(std::is_standard_layout_v<GPUCascadeShadowData>,
        "GPUCascadeShadowData must be standard-layout for shader interop");
    static_assert(std::is_standard_layout_v<GPUShadowData>,
        "GPUShadowData must be standard-layout for shader interop");
    static_assert(sizeof(GPUCascadeShadowData) == 80,
        "GPUCascadeShadowData must be Mat4 (64B) + Vec4 (16B) = 80B");
    static_assert(sizeof(GPUShadowData) == 16 + MaxShadowCascades * 80,
        "GPUShadowData size must match Slang-side layout");
}