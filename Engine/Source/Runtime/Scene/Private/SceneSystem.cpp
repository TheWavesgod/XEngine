#include <XEngine/Scene/SceneSystem.h>

#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Input/InputSystem.h>
#include <XEngine/Logging/Log.h>

#include <algorithm>

namespace XEngine
{
    SceneSystem::SceneSystem() = default;

    SceneSystem::~SceneSystem()
    {
        OnDestroy();
    }

    void SceneSystem::OnCreate(const SubsystemContext& context)
    {
        (void)context;
        if (m_Initialized)
        {
            return;
        }

        if (context.Engine != nullptr)
        {
            m_InputSystem = context.Engine->GetSubsystemManager().GetSubsystem<InputSystem>();
        }

        Scene& scene = CreateEmptyScene();
        m_DebugCameraEntity = scene.CreateEntity("DebugCamera");
        TransformComponent& transform = scene.AddTransform(m_DebugCameraEntity);
        transform.Position = Vec3 { 0.0f, 1.0f, 3.0f };
        transform.Dirty = true;

        CameraComponent& camera = scene.AddCamera(m_DebugCameraEntity);
        camera.Primary = true;
        m_DebugCameraController.Attach(&scene, m_DebugCameraEntity);

        m_Initialized = true;
        XENGINE_LOG_INFO("SceneSystem initialized");
    }

    void SceneSystem::OnDestroy()
    {
        if (!m_Initialized && !m_ActiveScene)
        {
            return;
        }

        m_ActiveScene.reset();
        m_DebugCameraController.Detach();
        m_DebugCameraEntity = {};
        m_InputSystem = nullptr;
        m_Initialized = false;
        XENGINE_LOG_INFO("SceneSystem shutdown");
    }

    void SceneSystem::OnUpdate(float deltaTime)
    {
        (void)deltaTime;
        if (m_ActiveScene && m_InputSystem != nullptr)
        {
            m_DebugCameraController.Update(deltaTime, *m_InputSystem);
        }

        if (m_ActiveScene)
        {
            m_ActiveScene->UpdateTransforms();
        }
    }

    Scene* SceneSystem::GetActiveScene()
    {
        return m_ActiveScene.get();
    }

    const Scene* SceneSystem::GetActiveScene() const
    {
        return m_ActiveScene.get();
    }

    Scene& SceneSystem::CreateEmptyScene()
    {
        m_ActiveScene = std::make_unique<Scene>();
        m_DebugCameraController.Detach();
        m_DebugCameraEntity = {};
        return *m_ActiveScene;
    }

    void SceneSystem::FrameDebugCamera(const Vec3& center, float radius)
    {
        if (!m_ActiveScene)
        {
            return;
        }

        m_DebugCameraController.FrameBounds(center, radius);
        XENGINE_LOG_INFO("Stage 7G debug camera framed scene bounds.");

        CameraComponent* camera = GetPrimaryCamera();
        if (camera != nullptr)
        {
            radius = std::max(radius, 0.5f);
            camera->NearPlane = std::max(0.01f, radius * 0.001f);
            camera->FarPlane = std::max(100.0f, radius * 10.0f);
        }
    }

    Entity SceneSystem::GetPrimaryCameraEntity() const
    {
        return m_DebugCameraEntity;
    }

    CameraComponent* SceneSystem::GetPrimaryCamera()
    {
        return m_ActiveScene ? m_ActiveScene->GetCamera(m_DebugCameraEntity) : nullptr;
    }

    const CameraComponent* SceneSystem::GetPrimaryCamera() const
    {
        return m_ActiveScene ? m_ActiveScene->GetCamera(m_DebugCameraEntity) : nullptr;
    }

    TransformComponent* SceneSystem::GetPrimaryCameraTransform()
    {
        return m_ActiveScene ? m_ActiveScene->GetTransform(m_DebugCameraEntity) : nullptr;
    }

    const TransformComponent* SceneSystem::GetPrimaryCameraTransform() const
    {
        return m_ActiveScene ? m_ActiveScene->GetTransform(m_DebugCameraEntity) : nullptr;
    }
}
