#pragma once

#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Math/Rotator.h>

namespace XEngine
{
    class TransformSystem;

    struct TransformComponent
    {
    public:
        void SetLocalPosition(const Vec3& position)
        {
            m_LocalPosition = position;
            MarkDirty();
        }

        void SetLocalRotation(const Quat& rotation)
        {
            m_LocalRotation = Math::Normalize(rotation);
            MarkDirty();
        }

        void SetLocalRotationDegrees(const Math::Rotator& rotation)
        {
            SetLocalRotation(Math::ToQuat(rotation));
        }

        void SetLocalScale(const Vec3& scale)
        {
            m_LocalScale = scale;
            MarkDirty();
        }

        const Vec3& GetLocalPosition() const { return m_LocalPosition; }
        const Quat& GetLocalRotation() const { return m_LocalRotation; }
        Math::Rotator GetLocalRotationDegrees() const { return Math::ToRotator(m_LocalRotation); }
        const Vec3& GetLocalScale() const { return m_LocalScale; }

        const Vec3& GetWorldPosition() const { return m_WorldPosition; }
        const Quat& GetWorldRotation() const { return m_WorldRotation; }
        Math::Rotator GetWorldRotationDegrees() const { return Math::ToRotator(m_WorldRotation); }
        const Vec3& GetWorldScale() const { return m_WorldScale; }

        const Mat4& GetLocalMatrix() const { return m_LocalMatrix; }
        const Mat4& GetWorldMatrix() const { return m_WorldMatrix; }
        const Mat4& GetPreviousWorldMatrix() const { return m_PreviousWorldMatrix; }

        Vec3 GetForward() const { return Math::GetForwardVector(m_WorldRotation); }
        Vec3 GetRight() const { return Math::GetRightVector(m_WorldRotation); }
        Vec3 GetUp() const { return Math::GetUpVector(m_WorldRotation); }

        bool IsDirty() const { return m_Dirty; }
        void MarkDirty() { m_Dirty = true; }
        void ClearDirty() { m_Dirty = false; }

    private:
        Vec3 m_LocalPosition { 0.0f, 0.0f, 0.0f };
        Quat m_LocalRotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 m_LocalScale { 1.0f, 1.0f, 1.0f };

        Vec3 m_WorldPosition { 0.0f, 0.0f, 0.0f };
        Quat m_WorldRotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 m_WorldScale { 1.0f, 1.0f, 1.0f };

        Mat4 m_LocalMatrix { 1.0f };
        Mat4 m_WorldMatrix { 1.0f };
        Mat4 m_PreviousWorldMatrix { 1.0f };

        bool m_Dirty = true;

        friend class TransformSystem;
    };
}
