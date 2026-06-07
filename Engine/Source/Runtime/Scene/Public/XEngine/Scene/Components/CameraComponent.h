#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    struct TransformComponent;

    enum class CameraProjectionMode
    {
        Perspective,
        Orthographic
    };

    // Data-only Scene camera component.
    // Interactive debug camera input is intentionally deferred to Stage 7G.
    struct CameraComponent
    {
        CameraProjectionMode ProjectionMode = CameraProjectionMode::Perspective;

        float VerticalFovRadians = 1.04719755f;
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;

        float OrthographicHeight = 10.0f;
        bool Primary = false;
    };

    Mat4 BuildCameraViewMatrix(const TransformComponent& transform);
    Mat4 BuildCameraProjectionMatrix(const CameraComponent& camera, float aspectRatio);
}
