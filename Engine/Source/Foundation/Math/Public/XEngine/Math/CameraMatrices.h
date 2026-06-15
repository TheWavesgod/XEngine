#pragma once

#include <XEngine/Math/CoordinateSystem.h>

#include <cmath>

namespace XEngine::Math
{
    inline Mat4 LookAtLH_XForward(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        const Vec3 forward = glm::normalize(target - eye);
        const Vec3 right = glm::normalize(glm::cross(up, forward));
        const Vec3 cameraUp = glm::cross(forward, right);

        Mat4 view { 1.0f };
        view[0][0] = right.x;
        view[1][0] = right.y;
        view[2][0] = right.z;
        view[0][1] = cameraUp.x;
        view[1][1] = cameraUp.y;
        view[2][1] = cameraUp.z;
        view[0][2] = forward.x;
        view[1][2] = forward.y;
        view[2][2] = forward.z;
        view[3][0] = -glm::dot(right, eye);
        view[3][1] = -glm::dot(cameraUp, eye);
        view[3][2] = -glm::dot(forward, eye);
        return view;
    }

    inline Mat4 BuildViewMatrixLH_XForward(const Vec3& position, const Quat& rotation)
    {
        return LookAtLH_XForward(
            position,
            position + CoordinateSystem::GetForwardVector(rotation),
            CoordinateSystem::GetUpVector(rotation));
    }

    inline Mat4 PerspectiveLH_ZO(
        float verticalFovRadians,
        float aspectRatio,
        float nearPlane,
        float farPlane)
    {
        const float tanHalfFov = std::tan(verticalFovRadians * 0.5f);
        Mat4 projection { 0.0f };
        projection[0][0] = 1.0f / (aspectRatio * tanHalfFov);
        projection[1][1] = 1.0f / tanHalfFov;
        projection[2][2] = farPlane / (farPlane - nearPlane);
        projection[2][3] = 1.0f;
        projection[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
        return projection;
    }

    inline Mat4 OrthographicLH_ZO(
        float left,
        float right,
        float bottom,
        float top,
        float nearPlane,
        float farPlane)
    {
        Mat4 projection { 1.0f };
        projection[0][0] = 2.0f / (right - left);
        projection[1][1] = 2.0f / (top - bottom);
        projection[2][2] = 1.0f / (farPlane - nearPlane);
        projection[3][0] = -(right + left) / (right - left);
        projection[3][1] = -(top + bottom) / (top - bottom);
        projection[3][2] = -nearPlane / (farPlane - nearPlane);
        return projection;
    }
}

namespace XEngine
{
    using Math::BuildViewMatrixLH_XForward;
    using Math::LookAtLH_XForward;
    using Math::OrthographicLH_ZO;
    using Math::PerspectiveLH_ZO;
}
