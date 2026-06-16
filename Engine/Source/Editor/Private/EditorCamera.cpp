#include <XEngine/Editor/EditorCamera.h>

namespace XEngine
{
    void EditorCamera::SetPosition(const Vec3& position)
    {
        m_Position = position;
    }

    void EditorCamera::SetRotationDegrees(const Math::Rotator& rotationDegrees)
    {
        m_RotationDegrees = rotationDegrees;
    }

    const Vec3& EditorCamera::GetPosition() const
    {
        return m_Position;
    }

    Math::Rotator EditorCamera::GetRotationDegrees() const
    {
        return m_RotationDegrees;
    }

    Quat EditorCamera::GetRotation() const
    {
        return Math::ToQuat(m_RotationDegrees);
    }

    void EditorCamera::SetFovDegrees(float fovDegrees)
    {
        m_FovDegrees = Math::Clamp(fovDegrees, 10.0f, 140.0f);
    }

    void EditorCamera::SetNearPlane(float nearPlane)
    {
        m_NearPlane = Math::Max(nearPlane, 0.001f);
    }

    void EditorCamera::SetFarPlane(float farPlane)
    {
        m_FarPlane = Math::Max(farPlane, m_NearPlane + 1.0f);
    }

    Mat4 EditorCamera::GetViewMatrix() const
    {
        return Math::BuildViewMatrixLH_XForward(m_Position, GetRotation());
    }

    Mat4 EditorCamera::GetProjectionMatrix(float aspectRatio) const
    {
        return Math::PerspectiveLH_ZO(
            Math::Radians(m_FovDegrees),
            Math::Max(aspectRatio, 0.001f),
            m_NearPlane,
            m_FarPlane);
    }

    RenderView EditorCamera::BuildRenderView(float aspectRatio) const
    {
        RenderView view;
        view.View = GetViewMatrix();
        view.Projection = GetProjectionMatrix(aspectRatio);
        view.ViewProjection = view.Projection * view.View;
        view.Position = m_Position;
        view.NearPlane = m_NearPlane;
        view.FarPlane = m_FarPlane;
        return view;
    }
}
