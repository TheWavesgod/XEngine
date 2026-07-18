#pragma once

#include <XEngine/Math/CoordinateSystem.h>
#include <XEngine/Math/MathTypes.h>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace XEngine::Math
{
    // Stored as degrees. Roll/Pitch/Yaw rotate around +X/+Y/+Z respectively.
    // Rotation order in ToQuat is qYawZ * qPitchY * qRollX.
    struct Rotator
    {
        float Roll = 0.0f;
        float Pitch = 0.0f;
        float Yaw = 0.0f;

        // Explicit constructor enforcing the degree unit.
        constexpr Rotator(float rollDegrees, float pitchDegrees, float yawDegrees)
            : Roll(rollDegrees), Pitch(pitchDegrees), Yaw(yawDegrees)
        {
        }

        constexpr Rotator() = default;

        // Factory alias for call sites that want to be loud about the unit.
        static constexpr Rotator MakeDegrees(float roll, float pitch, float yaw)
        {
            return Rotator(roll, pitch, yaw);
        }
    };

    inline Quat ToQuat(const Rotator& rotator)
    {
        const Quat roll = glm::angleAxis(glm::radians(rotator.Roll), CoordinateSystem::Forward);
        // XEngine uses left-handed +X forward / +Y right / +Z up semantics.
        // GLM angleAxis uses right-handed rotation, so pitch is negated to keep
        // negative pitch looking downward in engine space.
        const Quat pitch = glm::angleAxis(glm::radians(-rotator.Pitch), CoordinateSystem::Right);
        const Quat yaw = glm::angleAxis(glm::radians(rotator.Yaw), CoordinateSystem::Up);

        // Apply roll, then pitch, then yaw: q = qYawZ * qPitchY * qRollX.
        return glm::normalize(yaw * pitch * roll);
    }

    inline Rotator ToRotator(const Quat& rotation)
    {
        const Mat4 matrix = glm::mat4_cast(glm::normalize(rotation));
        const float sinPitch = std::clamp(matrix[0][2], -1.0f, 1.0f);

        Rotator result;
        result.Pitch = glm::degrees(std::asin(sinPitch));

        if (std::abs(sinPitch) < 0.999999f)
        {
            result.Roll = glm::degrees(std::atan2(matrix[1][2], matrix[2][2]));
            result.Yaw = glm::degrees(std::atan2(matrix[0][1], matrix[0][0]));
        }
        else
        {
            result.Roll = 0.0f;
            result.Yaw = glm::degrees(std::atan2(-matrix[1][0], matrix[1][1]));
        }
        return result;
    }
}
