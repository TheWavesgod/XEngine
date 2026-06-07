#include <XEngine/Scene/DebugCameraController.h>

#include <XEngine/Input/InputSystem.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Scene.h>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace XEngine
{
    namespace
    {
        constexpr float MaxPitch = 1.55334306f;

        Vec3 GetForward(float yaw, float pitch)
        {
            return glm::normalize(Vec3 {
                std::cos(pitch) * std::sin(yaw),
                std::sin(pitch),
                -std::cos(pitch) * std::cos(yaw)
            });
        }
    }

    void DebugCameraController::Attach(Scene* scene, Entity cameraEntity)
    {
        m_Scene = scene;
        m_CameraEntity = cameraEntity;
        m_Attached = scene != nullptr && scene->IsValid(cameraEntity);

        if (m_Attached)
        {
            if (const TransformComponent* transform = m_Scene->GetTransform(m_CameraEntity))
            {
                UpdateOrientationFromTransform(*transform);
            }
        }
    }

    void DebugCameraController::Detach()
    {
        m_Scene = nullptr;
        m_CameraEntity = {};
        m_Attached = false;
    }

    void DebugCameraController::Update(float deltaTime, const InputSystem& input)
    {
        if (!m_Attached || m_Scene == nullptr)
        {
            return;
        }

        TransformComponent* transform = m_Scene->GetTransform(m_CameraEntity);
        if (transform == nullptr)
        {
            return;
        }

        const float wheel = input.GetMouseWheelDelta();
        if (wheel != 0.0f)
        {
            m_MoveSpeed = std::clamp(m_MoveSpeed * (1.0f + wheel * 0.1f), 0.05f, 500.0f);
        }

        if (!input.IsMouseButtonDown(MouseButton::Right))
        {
            return;
        }

        const Vec2 mouseDelta = input.GetMouseDelta();
        m_YawRadians -= mouseDelta.x * m_MouseSensitivity;
        m_PitchRadians -= mouseDelta.y * m_MouseSensitivity;
        m_PitchRadians = std::clamp(m_PitchRadians, -MaxPitch, MaxPitch);

        const Vec3 forward = GetForward(m_YawRadians, m_PitchRadians);
        const Vec3 right = glm::normalize(glm::cross(forward, Vec3 { 0.0f, 1.0f, 0.0f }));
        const Vec3 up { 0.0f, 1.0f, 0.0f };

        Vec3 movement { 0.0f, 0.0f, 0.0f };
        if (input.IsKeyDown(KeyCode::W))
        {
            movement += forward;
        }
        if (input.IsKeyDown(KeyCode::S))
        {
            movement -= forward;
        }
        if (input.IsKeyDown(KeyCode::D))
        {
            movement += right;
        }
        if (input.IsKeyDown(KeyCode::A))
        {
            movement -= right;
        }
        if (input.IsKeyDown(KeyCode::E))
        {
            movement += up;
        }
        if (input.IsKeyDown(KeyCode::Q))
        {
            movement -= up;
        }

        const float speedScale =
            (input.IsKeyDown(KeyCode::LeftShift) || input.IsKeyDown(KeyCode::RightShift)) ?
            m_FastMoveMultiplier :
            1.0f;

        if (glm::length(movement) > 0.0f)
        {
            transform->Position += glm::normalize(movement) * m_MoveSpeed * speedScale * deltaTime;
        }

        ApplyOrientationToTransform(*transform);
        transform->Dirty = true;
    }

    void DebugCameraController::FrameBounds(const Vec3& center, float radius)
    {
        if (!m_Attached || m_Scene == nullptr)
        {
            return;
        }

        TransformComponent* transform = m_Scene->GetTransform(m_CameraEntity);
        if (transform == nullptr)
        {
            return;
        }

        radius = std::max(radius, 0.5f);
        transform->Position = center + Vec3 { 0.0f, radius * 0.25f, radius * 2.5f };

        const Vec3 direction = glm::normalize(center - transform->Position);
        m_YawRadians = std::atan2(direction.x, -direction.z);
        m_PitchRadians = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
        m_MoveSpeed = std::max(0.5f, radius * 0.5f);

        ApplyOrientationToTransform(*transform);
        transform->Dirty = true;
    }

    Entity DebugCameraController::GetCameraEntity() const
    {
        return m_CameraEntity;
    }

    void DebugCameraController::SetMoveSpeed(float speed)
    {
        m_MoveSpeed = std::clamp(speed, 0.05f, 500.0f);
    }

    float DebugCameraController::GetMoveSpeed() const
    {
        return m_MoveSpeed;
    }

    void DebugCameraController::SetMouseSensitivity(float sensitivity)
    {
        m_MouseSensitivity = std::max(0.0001f, sensitivity);
    }

    float DebugCameraController::GetMouseSensitivity() const
    {
        return m_MouseSensitivity;
    }

    void DebugCameraController::UpdateOrientationFromTransform(const TransformComponent& transform)
    {
        const Vec3 forward = glm::normalize(transform.Rotation * Vec3 { 0.0f, 0.0f, -1.0f });
        m_YawRadians = std::atan2(forward.x, -forward.z);
        m_PitchRadians = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    }

    void DebugCameraController::ApplyOrientationToTransform(TransformComponent& transform) const
    {
        const Quat yaw = glm::angleAxis(m_YawRadians, Vec3 { 0.0f, 1.0f, 0.0f });
        const Quat pitch = glm::angleAxis(m_PitchRadians, Vec3 { 1.0f, 0.0f, 0.0f });
        transform.Rotation = yaw * pitch;
    }
}
