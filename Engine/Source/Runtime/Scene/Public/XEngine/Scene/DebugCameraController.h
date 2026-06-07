#pragma once

#include <XEngine/Math/MathTypes.h>
#include <XEngine/Scene/Entity.h>

namespace XEngine
{
    class InputSystem;
    class Scene;
    struct TransformComponent;

    // UE-style debug camera controller for scene inspection.
    // It updates a Scene camera entity from InputSystem state and is not a gameplay camera.
    class DebugCameraController
    {
    public:
        void Attach(Scene* scene, Entity cameraEntity);
        void Detach();

        void Update(float deltaTime, const InputSystem& input);
        void FrameBounds(const Vec3& center, float radius);

        Entity GetCameraEntity() const;

        void SetMoveSpeed(float speed);
        float GetMoveSpeed() const;

        void SetMouseSensitivity(float sensitivity);
        float GetMouseSensitivity() const;

    private:
        void UpdateOrientationFromTransform(const TransformComponent& transform);
        void ApplyOrientationToTransform(TransformComponent& transform) const;

        Scene* m_Scene = nullptr;
        Entity m_CameraEntity {};
        float m_YawRadians = 0.0f;
        float m_PitchRadians = 0.0f;
        float m_MoveSpeed = 3.0f;
        float m_FastMoveMultiplier = 4.0f;
        float m_MouseSensitivity = 0.0025f;
        bool m_Attached = false;
    };
}
