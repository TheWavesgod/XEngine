#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine::CoordinateSystem
{
    // XEngine world space is left-handed: +X forward, +Y right, +Z up.
    // Graphics API clip-space differences are adapted at the projection/RHI boundary.
    inline const Vec3 Forward { 1.0f, 0.0f, 0.0f };
    inline const Vec3 Right { 0.0f, 1.0f, 0.0f };
    inline const Vec3 Up { 0.0f, 0.0f, 1.0f };

    inline Vec3 GetForwardVector(const Quat& rotation)
    {
        return glm::normalize(rotation * Forward);
    }

    inline Vec3 GetRightVector(const Quat& rotation)
    {
        return glm::normalize(rotation * Right);
    }

    inline Vec3 GetUpVector(const Quat& rotation)
    {
        return glm::normalize(rotation * Up);
    }
}

namespace XEngine::Math
{
    inline Vec3 GetForwardVector(const Quat& rotation)
    {
        return CoordinateSystem::GetForwardVector(rotation);
    }

    inline Vec3 GetRightVector(const Quat& rotation)
    {
        return CoordinateSystem::GetRightVector(rotation);
    }

    inline Vec3 GetUpVector(const Quat& rotation)
    {
        return CoordinateSystem::GetUpVector(rotation);
    }
}

namespace XEngine::Units
{
    inline constexpr float MetersPerUnit = 1.0f;
    inline constexpr float UnitsPerMeter = 1.0f;

    inline constexpr float CentimetersPerUnit = 100.0f;
    inline constexpr float UnitsPerCentimeter = 0.01f;
}