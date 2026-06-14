#pragma once

#include <XEngine/Math/MathTypes.h>

#include <cstddef>
#include <type_traits>

namespace XEngine
{
    static_assert(sizeof(Mat4) == sizeof(float) * 16);
    static_assert(sizeof(Vec4) == sizeof(float) * 4);

    struct alignas(16) PBRPushConstants
    {
        Mat4 ModelViewProjection { 1.0f };
        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4 MaterialFactors { 0.0f, 1.0f, 0.5f, 0.0f };
    };

    struct alignas(16) MeshPushConstants
    {
        Mat4 ModelViewProjection { 1.0f };
    };

    static_assert(std::is_standard_layout_v<PBRPushConstants>);
    static_assert(offsetof(PBRPushConstants, BaseColorFactor) == sizeof(Mat4));
    static_assert(offsetof(PBRPushConstants, MaterialFactors) == sizeof(Mat4) + sizeof(Vec4));
    static_assert(sizeof(PBRPushConstants) == sizeof(float) * 24);
    static_assert(sizeof(MeshPushConstants) == sizeof(float) * 16);
}
