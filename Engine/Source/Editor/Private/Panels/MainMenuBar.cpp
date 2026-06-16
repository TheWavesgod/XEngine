#include "MainMenuBar.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/LightComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Scene.h>
#include <XEngine/Scene/SceneSerializer.h>
#include <XEngine/Serialization/SerializationContext.h>

#include <imgui.h>

namespace XEngine
{
    namespace
    {
        constexpr const char* DefaultScenePath = "Assets/Scenes/Default.xscene";
        constexpr const char* ValidationScenePath = "Assets/Scenes/ValidationScene.xscene";
        constexpr const char* ShadowValidationScenePath = "Assets/Scenes/ShadowValidation.xscene";

        const char* SceneDisplayName(const EditorContext& context)
        {
            static std::string displayName;
            displayName = context.CurrentScenePath.empty() ?
                "Untitled" :
                context.CurrentScenePath.filename().generic_string();
            if (context.SceneDirty)
            {
                displayName += " *";
            }
            return displayName.c_str();
        }
    }

    void MainMenuBar::Draw(EditorContext& context)
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        ImGui::TextUnformatted(SceneDisplayName(context));
        ImGui::Separator();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                NewScene(context);
            }
            if (ImGui::MenuItem("Open Default Scene"))
            {
                OpenScene(context, DefaultScenePath);
            }
            if (ImGui::MenuItem("Open Validation Scene"))
            {
                OpenScene(context, ValidationScenePath);
            }
            if (ImGui::MenuItem("Open Shadow Validation Scene"))
            {
                OpenScene(context, ShadowValidationScenePath);
            }
            if (ImGui::MenuItem("Save Scene"))
            {
                SaveScene(context, DefaultScenePath);
            }
            if (ImGui::MenuItem("Save Scene As Shadow Validation Scene"))
            {
                SaveSceneAs(context, ShadowValidationScenePath);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene Hierarchy", nullptr, &context.ShowSceneHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &context.ShowInspector);
            ImGui::MenuItem("Renderer Debug", nullptr, &context.ShowRendererDebug);
            ImGui::MenuItem("Viewport", nullptr, &context.ShowViewport);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::MenuItem("Use Editor Camera", nullptr, &context.UseEditorCamera) &&
                !context.UseEditorCamera)
            {
                context.ViewportInputMode = ViewportInputMode::UI;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void MainMenuBar::NewScene(EditorContext& context)
    {
        Scene* scene = context.ActiveScene;
        if (scene == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create new editor scene without an active Scene");
            return;
        }

        scene->Clear();

        Entity cameraEntity = scene->CreateEntity("Main Camera");
        TransformComponent& cameraTransform = scene->AddTransform(cameraEntity);
        cameraTransform.SetLocalPosition(Vec3 { -4.0f, 0.0f, 2.0f });
        cameraTransform.SetLocalRotationDegrees(Math::Rotator { 0.0f, -20.0f, 0.0f });
        CameraComponent& camera = scene->AddCamera(cameraEntity);
        camera.Primary = true;

        Entity lightEntity = scene->CreateEntity("Directional Light");
        TransformComponent& lightTransform = scene->AddTransform(lightEntity);
        lightTransform.SetLocalRotationDegrees(Math::Rotator { 0.0f, -45.0f, 135.0f });
        LightComponent& light = scene->AddLight(lightEntity);
        light.Type = LightType::Directional;
        light.Intensity = 4.0f;

        if (context.Assets != nullptr)
        {
            Entity cubeEntity = scene->CreateEntity("Ground Cube");
            TransformComponent& cubeTransform = scene->AddTransform(cubeEntity);
            cubeTransform.SetLocalPosition(Vec3 { 0.0f, 0.0f, -0.5f });
            cubeTransform.SetLocalScale(Vec3 { 3.0f, 3.0f, 0.2f });
            MeshRendererComponent& renderer = scene->AddMeshRenderer(cubeEntity);
            renderer.MeshAsset = context.Assets->CreateProceduralCubeMeshAsset("EditorDefaultCube");
            renderer.MaterialAsset = context.Assets->CreateTestMaterialAsset("EditorDefaultMaterial", {});
        }

        scene->UpdateTransforms();
        context.SelectedEntity = {};
        context.CurrentScenePath = DefaultScenePath;
        context.SceneDirty = true;
        XENGINE_LOG_INFO("Created new editor scene");
    }

    void MainMenuBar::OpenScene(EditorContext& context, const char* path)
    {
        if (context.ActiveScene == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot load scene without an active Scene");
            return;
        }

        SerializationContext serializationContext;
        serializationContext.Assets = context.Assets;

        // Editor calls Runtime SceneSerializer and does not own the .xscene
        // format; schema changes belong in the Scene serialization layer.
        SceneSerializer serializer(serializationContext);
        if (!serializer.LoadFromFile(*context.ActiveScene, path))
        {
            XENGINE_LOG_ERROR(std::string("Failed to load scene: ") + path);
            return;
        }

        context.SelectedEntity = {};
        context.CurrentScenePath = path;
        context.SceneDirty = false;
        XENGINE_LOG_INFO(std::string("Loaded scene: ") + path);
    }

    void MainMenuBar::SaveScene(EditorContext& context, const char* fallbackPath)
    {
        const std::filesystem::path path =
            context.CurrentScenePath.empty() ? std::filesystem::path(fallbackPath) : context.CurrentScenePath;
        SaveSceneAs(context, path.generic_string().c_str());
    }

    void MainMenuBar::SaveSceneAs(EditorContext& context, const char* path)
    {
        if (context.ActiveScene == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot save scene without an active Scene");
            return;
        }

        SerializationContext serializationContext;
        serializationContext.Assets = context.Assets;

        // Editor delegates scene file IO to Runtime SceneSerializer; panel code
        // only chooses fixed paths for this stage.
        SceneSerializer serializer(serializationContext);
        if (!serializer.SaveToFile(*context.ActiveScene, path))
        {
            XENGINE_LOG_ERROR(std::string("Failed to save scene: ") + path);
            return;
        }

        context.CurrentScenePath = path;
        context.SceneDirty = false;
        XENGINE_LOG_INFO(std::string("Saved scene: ") + path);
    }
}
