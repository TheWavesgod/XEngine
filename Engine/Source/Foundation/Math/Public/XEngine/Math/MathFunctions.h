#pragma once

#include <XEngine/Math/MathTypes.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace XEngine
{
    inline float Radians(float degrees)
    {
        return glm::radians(degrees);
    }

    inline float Degrees(float radians)
    {
        return glm::degrees(radians);
    }

    inline Mat4 Perspective(float fovRadians, float aspect, float nearPlane, float farPlane)
    {
        Mat4 result = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
        result[1][1] *= -1.0f;
        return result;
    }

    inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
    {
        return glm::lookAt(eye, center, up);
    }

    inline Mat4 Translate(const Vec3& translation)
    {
        return glm::translate(Mat4(1.0f), translation);
    }

    inline Mat4 Rotate(const Quat& rotation)
    {
        return glm::toMat4(rotation);
    }

    inline Mat4 Scale(const Vec3& scale)
    {
        return glm::scale(Mat4(1.0f), scale);
    }
}
