#include "FreeCameraController.h"

#include <XEngine/Editor/EditorCamera.h>
#include <XEngine/Math/CoordinateSystem.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Platform/PlatformEvents.h>

namespace XEngine
{
    void FreeCameraController::ProcessEvent(const PlatformEvent& event)
    {
        const bool pressed = event.Type == PlatformEventType::KeyDown;
        if (event.Type == PlatformEventType::KeyDown || event.Type == PlatformEventType::KeyUp)
        {
            switch (event.Key)
            {
            case KeyCode::W:
                m_Forward = pressed;
                break;
            case KeyCode::S:
                m_Backward = pressed;
                break;
            case KeyCode::A:
                m_Left = pressed;
                break;
            case KeyCode::D:
                m_Right = pressed;
                break;
            case KeyCode::Q:
                m_Down = pressed;
                break;
            case KeyCode::E:
                m_Up = pressed;
                break;
            case KeyCode::LeftShift:
            case KeyCode::RightShift:
                m_Fast = pressed;
                break;
            default:
                break;
            }
        }
        else if (event.Type == PlatformEventType::MouseMove)
        {
            m_MouseDelta += Vec2 { event.MouseDeltaX, event.MouseDeltaY };
        }
    }

    void FreeCameraController::Update(EditorCamera& camera, float deltaTime, bool active)
    {
        // EditorCamera input is processed only while the viewport has explicitly
        // captured input; generic ImGui capture flags are not the camera mode.
        if (!active)
        {
            ResetTransientInput();
            return;
        }

        Math::Rotator rotation = camera.GetRotationDegrees();
        rotation.Yaw += m_MouseDelta.x * m_MouseSensitivity;
        rotation.Pitch = Math::Clamp(
            rotation.Pitch - m_MouseDelta.y * m_MouseSensitivity,
            -89.0f,
            89.0f);
        rotation.Roll = 0.0f;
        camera.SetRotationDegrees(rotation);

        const Quat orientation = camera.GetRotation();
        Vec3 movement { 0.0f, 0.0f, 0.0f };
        if (m_Forward)
        {
            movement += CoordinateSystem::GetForwardVector(orientation);
        }
        if (m_Backward)
        {
            movement -= CoordinateSystem::GetForwardVector(orientation);
        }
        if (m_Right)
        {
            movement += CoordinateSystem::GetRightVector(orientation);
        }
        if (m_Left)
        {
            movement -= CoordinateSystem::GetRightVector(orientation);
        }
        if (m_Up)
        {
            movement += CoordinateSystem::Up;
        }
        if (m_Down)
        {
            movement -= CoordinateSystem::Up;
        }

        if (Math::LengthSquared(movement) > 0.000001f)
        {
            movement = Math::Normalize(movement);
            const float speed = m_MoveSpeed * (m_Fast ? m_FastMultiplier : 1.0f);
            camera.SetPosition(camera.GetPosition() + movement * speed * deltaTime);
        }

        ResetTransientInput();
    }

    void FreeCameraController::ResetTransientInput()
    {
        m_MouseDelta = {};
    }
}
