#pragma once

#include <XEngine/Math/CameraMatrices.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace XEngine::Math
{
    template<typename T>
    inline auto Dot(const T& lhs, const T& rhs)
    {
        return glm::dot(lhs, rhs);
    }

    template<typename T>
    inline T Cross(const T& lhs, const T& rhs)
    {
        return glm::cross(lhs, rhs);
    }

    template<typename T>
    inline T Normalize(const T& value)
    {
        return glm::normalize(value);
    }

    template<typename T>
    inline auto Length(const T& value)
    {
        return glm::length(value);
    }

    template<typename T>
    inline auto LengthSquared(const T& value)
    {
        return glm::dot(value, value);
    }

    template<typename T>
    inline T Inverse(const T& value)
    {
        return glm::inverse(value);
    }

    template<typename T>
    inline T Transpose(const T& value)
    {
        return glm::transpose(value);
    }

    inline float Radians(float degrees)
    {
        return glm::radians(degrees);
    }

    inline float Degrees(float radians)
    {
        return glm::degrees(radians);
    }

    template<typename T>
    inline T Clamp(const T& value, const T& minimum, const T& maximum)
    {
        return glm::clamp(value, minimum, maximum);
    }

    template<typename T, typename U>
    inline T Lerp(const T& from, const T& to, const U& alpha)
    {
        return glm::mix(from, to, alpha);
    }

    template<typename T>
    inline T Min(const T& lhs, const T& rhs)
    {
        return glm::min(lhs, rhs);
    }

    template<typename T>
    inline T Max(const T& lhs, const T& rhs)
    {
        return glm::max(lhs, rhs);
    }

    inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
    {
        return LookAtLH_XForward(eye, center, up);
    }

    inline Mat4 Translate(const Vec3& translation)
    {
        return glm::translate(Mat4(1.0f), translation);
    }

    inline Mat4 Rotate(const Quat& rotation)
    {
        return glm::mat4_cast(rotation);
    }

    inline Mat4 Scale(const Vec3& scale)
    {
        return glm::scale(Mat4(1.0f), scale);
    }

    inline Mat4 ComposeTRS(
        const Vec3& position,
        const Quat& rotation,
        const Vec3& scaleValue)
    {
        return Translate(position) * Rotate(rotation) * Scale(scaleValue);
    }

    inline Vec3 ExtractTranslation(const Mat4& matrix)
    {
        return Vec3 { matrix[3] };
    }

    inline Quat AngleAxis(float angleRadians, const Vec3& axis)
    {
        return glm::angleAxis(angleRadians, axis);
    }

    inline Vec3 TransformPoint(const Mat4& transform, const Vec3& point)
    {
        return Vec3(transform * Vec4(point, 1.0f));
    }

    inline Vec3 TransformVector(const Mat4& transform, const Vec3& vector)
    {
        return Vec3(transform * Vec4(vector, 0.0f));
    }
}

namespace XEngine
{
    using Math::AngleAxis;
    using Math::Clamp;
    using Math::Cross;
    using Math::Degrees;
    using Math::Dot;
    using Math::Inverse;
    using Math::Length;
    using Math::LengthSquared;
    using Math::Lerp;
    using Math::LookAt;
    using Math::Max;
    using Math::Min;
    using Math::PerspectiveLH_ZO;
    using Math::Radians;
    using Math::Rotate;
    using Math::Scale;
    using Math::TransformPoint;
    using Math::TransformVector;
    using Math::Translate;
    using Math::Transpose;
}
