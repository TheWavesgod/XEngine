#include <XEngine/Engine/Engine.h>
#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Scene/SceneSerializer.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Serialization/SerializationContext.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";
    config.WindowWidth = 1280;
    config.WindowHeight = 720;
    config.WindowResizable = true;
    config.CreateMainWindow = true;
    config.MaxFrames = 0;

    XEngine::Engine engine;
    engine.Initialize(config);

    XEngine::SubsystemManager& subsystems = engine.GetSubsystemManager();
    XEngine::SceneSystem* sceneSystem = subsystems.GetSubsystem<XEngine::SceneSystem>();
    XEngine::AssetSystem* assetSystem = subsystems.GetSubsystem<XEngine::AssetSystem>();
    XEngine::Scene* scene = sceneSystem != nullptr ? sceneSystem->GetActiveScene() : nullptr;
    if (scene != nullptr)
    {
        XEngine::SerializationContext serializationContext;
        serializationContext.Assets = assetSystem;

        // Sandbox validation: this path remains runtime-only, loads scene assets,
        // and must not depend on Editor or ImGui save workflows.
        XEngine::SceneSerializer serializer(serializationContext);
        if (!serializer.LoadFromFile(*scene, "Assets/Scenes/Default.xscene"))
        {
            XENGINE_LOG_ERROR("Failed to load Assets/Scenes/Default.xscene");
        }
    }

    // Sandbox uses the primary CameraComponent loaded from the Scene; EditorCamera
    // is editor-only and is not linked into this target.
    engine.Run();
    engine.Shutdown();

    return 0;
}
