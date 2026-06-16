#pragma once

#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Math/Rotator.h>
#include <XEngine/Renderer/RenderView.h>

namespace XEngine
{
    // Editor-only viewport camera. It is not a Scene component and is never
    // serialized into .xscene files.
    class EditorCamera
    {
    public:
        void SetPosition(const Vec3& position);
        void SetRotationDegrees(const Math::Rotator& rotationDegrees);

        const Vec3& GetPosition() const;
        Math::Rotator GetRotationDegrees() const;
        Quat GetRotation() const;

        void SetFovDegrees(float fovDegrees);
        void SetNearPlane(float nearPlane);
        void SetFarPlane(float farPlane);

        Mat4 GetViewMatrix() const;
        Mat4 GetProjectionMatrix(float aspectRatio) const;
        RenderView BuildRenderView(float aspectRatio) const;

    private:
        Vec3 m_Position { -4.0f, 0.0f, 2.0f };
        Math::Rotator m_RotationDegrees { 0.0f, -15.0f, 0.0f };
        float m_FovDegrees = 60.0f;
        float m_NearPlane = 0.1f;
        float m_FarPlane = 500.0f;
    };
}
