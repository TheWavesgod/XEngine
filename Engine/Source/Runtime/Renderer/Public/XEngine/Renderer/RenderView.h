#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    // Renderer consumes view data and does not own or know whether it came from
    // a runtime CameraComponent, an editor tool camera, or a future replay view.
    struct RenderView
    {
        Mat4 View { 1.0f };
        Mat4 Projection { 1.0f };
        Mat4 ViewProjection { 1.0f };
        Vec3 Position { 0.0f, 0.0f, 0.0f };
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;
    };
}
