#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine::CoordinateConversion
{
    inline Vec3 GltfPositionToXEngine(const Vec3& value)
    {
        return { -value.z, value.x, value.y };
    }

    inline Vec3 GltfDirectionToXEngine(const Vec3& value)
    {
        return glm::normalize(Vec3 { -value.z, value.x, value.y });
    }

    inline Vec4 GltfTangentToXEngine(const Vec4& tangent)
    {
        const Vec3 direction = GltfDirectionToXEngine(Vec3 { tangent });
        // The basis conversion changes handedness, so tangent-space handedness changes too.
        // Validate this convention with additional normal-mapped assets as the material system grows.
        return { direction, -tangent.w };
    }

    inline constexpr bool GltfToXEngineFlipsHandedness()
    {
        return true;
    }
}
