#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Scene/DebugCameraController.h>
#include <XEngine/Scene/Scene.h>

#include <memory>

namespace XEngine
{
    class InputSystem;

    // Runtime subsystem that owns the active Scene.
    // It does not render and does not own GPU resources.
    class SceneSystem final : public ISubsystem
    {
    public:
        SceneSystem();
        ~SceneSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

        Scene* GetActiveScene();
        const Scene* GetActiveScene() const;

        Scene& CreateEmptyScene();
        void FrameDebugCamera(const Vec3& center, float radius);

        Entity GetPrimaryCameraEntity() const;
        CameraComponent* GetPrimaryCamera();
        const CameraComponent* GetPrimaryCamera() const;
        TransformComponent* GetPrimaryCameraTransform();
        const TransformComponent* GetPrimaryCameraTransform() const;

    private:
        std::unique_ptr<Scene> m_ActiveScene;
        InputSystem* m_InputSystem = nullptr;
        DebugCameraController m_DebugCameraController;
        Entity m_DebugCameraEntity {};
        bool m_Initialized = false;
    };
}
