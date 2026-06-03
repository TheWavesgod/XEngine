#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class RHIBackend
    {
        None,
        Vulkan,
        D3D12,
        Metal
    };

    enum class RHIQueueType
    {
        Graphics,
        Compute,
        Transfer
    };

    enum class RHIFormat
    {
        Undefined,
        BGRA8Unorm,
        RGBA8Unorm,
        RGBA16Float,
        D32Float
    };

    struct RHIColor
    {
        f32 R = 0.0f;
        f32 G = 0.0f;
        f32 B = 0.0f;
        f32 A = 1.0f;
    };
}
