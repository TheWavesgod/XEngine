#pragma once

#include "GPULightingTypes.h"

#include <XEngine/Math/MathTypes.h>

#include <type_traits>

namespace XEngine
{
    // Shader interop type.
    // Must stay layout-compatible with Engine/Shaders/Common/Types.slang.
    struct alignas(16) GPUCameraData
    {
        Mat4 View { 1.0f };
        Mat4 Projection { 1.0f };
        Mat4 ViewProjection { 1.0f };

        // xyz = camera world position, w = padding.
        Vec4 CameraPosition { 0.0f };
    };

    // Shader interop type.
    // Must stay layout-compatible with Engine/Shaders/Common/Types.slang.
    struct alignas(16) GPUFrameData
    {
        GPUCameraData Camera;
        GPULightingData Lighting;
    };

    static_assert(sizeof(Mat4) == sizeof(float) * 16);
    static_assert(sizeof(Vec4) == sizeof(float) * 4);
    static_assert(std::is_standard_layout_v<GPUCameraData>);
    static_assert(std::is_standard_layout_v<GPUFrameData>);
    static_assert(sizeof(GPUCameraData) % 16 == 0);
    static_assert(sizeof(GPUFrameData) % 16 == 0);
}
