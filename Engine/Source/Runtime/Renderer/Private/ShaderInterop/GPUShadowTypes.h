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
}