#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine::CoordinateConversion
{
    // glTF uses right-handed, +Y up, -Z forward conventions.
    // XEngine uses left-handed, +Z up, +X forward.
    // Mapping glTF -> XEngine is a per-vertex axis swap: (x_g, y_g, z_g) -> (-z_g, x_g, y_g).
    // This mapping flips handedness, which in turn flips tangent-space handedness.
    inline Vec3 GltfPositionToXEngine(const Vec3& value)
    {
        return { -value.z, value.x, value.y };
    }

    inline Vec3 GltfDirectionToXEngine(const Vec3& value)
    {
        return glm::normalize(Vec3 { -value.z, value.x, value.y });
    }

    // Tangent conversion also flips the handedness bit (`w`).
    // NOTE: Validate this convention against a normal-mapped reference asset
    // (e.g. glTF-Sample-Models/Avocado) before relying on it in production.
    // The handedness choice is currently commented as "still needs validation"
    // and the rest of the engine treats `w = +1` / `-1` as the polarity test.
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
