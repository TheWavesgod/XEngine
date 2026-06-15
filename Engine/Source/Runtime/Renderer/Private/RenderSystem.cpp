#include <XEngine/Renderer/RenderSystem.h>

#include "Pipeline/ForwardRenderPipeline.h"
#include "Pipeline/RenderFrameContext.h"
#include "Pipeline/RenderPipeline.h"
#include "Pipeline/RenderProjection.h"
#include "Resources/RenderMeshManager.h"
#include "Resources/RenderMaterialSystem.h"
#include "Resources/RenderFrameResources.h"
#include "Resources/RenderPipelineStateCache.h"
#include "Resources/RenderResourceContext.h"
#include "Resources/RenderShaderLibrary.h"
#include "Resources/RenderTextureManager.h"
#include "Scene/RenderExtraction.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Core/Colors.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/CameraMatrices.h>
#include <XEngine/Math/CoordinateSystem.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/Scene/Scene.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Shader/ShaderSystem.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace XEngine
{
    namespace
    {
        bool FindFirstMeshAndMaterial(
            const AssetImportResult& importResult,
            const AssetSystem& assetSystem,
            AssetHandle& outMesh,
            AssetHandle& outMaterial)
        {
            for (AssetHandle handle : importResult.ImportedAssets)
            {
                if (!outMesh.IsValid() && assetSystem.GetMeshAsset(handle) != nullptr)
                {
                    outMesh = handle;
                }
                if (!outMaterial.IsValid() && assetSystem.GetMaterialAsset(handle) != nullptr)
                {
                    outMaterial = handle;
                }
            }
            return outMesh.IsValid() && outMaterial.IsValid();
        }

        void CreateValidationSceneEntity(
            Scene& scene,
            const char* name,
            AssetHandle meshAsset,
            AssetHandle materialAsset)
        {
            Entity entity = scene.CreateEntity(name != nullptr ? name : "Renderable");
            TransformComponent& transform = scene.AddTransform(entity);
            transform.SetLocalPosition(Vec3 { 0.0f, 0.0f, 0.0f });
            transform.SetLocalRotationDegrees(Math::Rotator { 0.0f, 90.0f, 0.0f });
            transform.SetLocalScale(Vec3 { 1.0f, 1.0f, 1.0f });

            MeshRendererComponent& renderer = scene.AddMeshRenderer(entity);
            renderer.MeshAsset = meshAsset;
            renderer.MaterialAsset = materialAsset;
        }

        std::filesystem::path FindGltfValidationAsset(
            const char* preferredName,
            const std::filesystem::path& fallback)
        {
            if (std::filesystem::exists(fallback))
            {
                return fallback;
            }

            const std::filesystem::path root = "Assets/models/gltf";
            if (!std::filesystem::exists(root))
            {
                return {};
            }

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(root))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                const std::filesystem::path path = entry.path();
                const std::string generic = path.generic_string();
                if ((path.extension() == ".gltf" || path.extension() == ".glb") &&
                    generic.find(preferredName) != std::string::npos)
                {
                    return path;
                }
            }
            return {};
        }

        void FrameCameraForMesh(SceneSystem& sceneSystem, const MeshAsset& meshAsset)
        {
            const Vec3 center = meshAsset.Bounds.GetCenter();
            const float radius = std::max(Math::Length(meshAsset.Bounds.GetExtents()), 0.5f);
            sceneSystem.FrameDebugCamera(center, radius);
        }
    }

    struct RenderSystem::Impl
    {
        Engine* EngineInstance = nullptr;
        RHISystem* RHI = nullptr;
        ShaderSystem* Shader = nullptr;
        AssetSystem* Assets = nullptr;
        SceneSystem* Scenes = nullptr;

        std::unique_ptr<RenderTextureManager> Textures;
        std::unique_ptr<RenderMeshManager> Meshes;
        std::unique_ptr<RenderMaterialSystem> Materials;
        std::unique_ptr<RenderShaderLibrary> Shaders;
        std::unique_ptr<RenderFrameResources> FrameResources;
        std::unique_ptr<RenderPipelineStateCache> PipelineStates;
        RenderResourceContext Resources;

        std::unique_ptr<RenderPipeline> ActivePipeline;
        RenderScene SceneData;

        Mat4 FallbackViewProjection { 1.0f };
        u32 SwapchainWidth = 1280;
        u32 SwapchainHeight = 720;
        bool Initialized = false;

        void Shutdown()
        {
            if (RHI != nullptr)
            {
                RHIDevice* device = RHI->GetDevice();
                if (device != nullptr && device->IsValid())
                {
                    device->WaitIdle();
                }
            }

            if (ActivePipeline)
            {
                ActivePipeline->Shutdown();
                ActivePipeline.reset();
            }
            if (FrameResources)
            {
                FrameResources->Shutdown();
                FrameResources.reset();
            }
            if (PipelineStates)
            {
                PipelineStates->Shutdown();
                PipelineStates.reset();
            }
            if (Shaders)
            {
                Shaders->Shutdown();
                Shaders.reset();
            }
            if (Materials)
            {
                Materials->Shutdown();
                Materials.reset();
            }
            if (Meshes)
            {
                Meshes->Shutdown();
                Meshes.reset();
            }
            if (Textures)
            {
                Textures->Shutdown();
                Textures.reset();
            }

            Resources = {};
            SceneData.Clear();
            Scenes = nullptr;
            Assets = nullptr;
            Shader = nullptr;
            RHI = nullptr;
            EngineInstance = nullptr;
            Initialized = false;
        }

        void CreateValidationScene()
        {
            AssetHandle baseColorTextureAsset;
            const std::string checkerPath = "Assets/Textures/checker.jpg";
            if (Assets != nullptr && std::filesystem::exists(checkerPath))
            {
                AssetImportResult checkerImport = Assets->ImportAsset(checkerPath);
                if (checkerImport.Succeeded() && Assets->GetTextureAsset(checkerImport.MainAsset) != nullptr)
                {
                    baseColorTextureAsset = checkerImport.MainAsset;
                }
            }

            Scene* activeScene = Scenes != nullptr ? Scenes->GetActiveScene() : nullptr;
            if (Assets == nullptr || activeScene == nullptr)
            {
                XENGINE_LOG_WARN("Stage 8A validation scene skipped because AssetSystem or SceneSystem is unavailable.");
                return;
            }

            const Entity lightEntity = activeScene->CreateEntity("Stage8_DirectionalLight");
            TransformComponent& lightTransform = activeScene->AddTransform(lightEntity);
            lightTransform.SetLocalPosition(Vec3 { 0.0f, 0.0f, 0.0f });
            lightTransform.SetLocalRotationDegrees(Math::Rotator { 0.0f, -45.0f, 135.0f });

            LightComponent& light = activeScene->AddLight(lightEntity);
            light.Type = LightType::Directional;
            light.Color = Colors::Sunlight;
            light.Intensity = 3.0f;
            light.CastShadow = true;

            const std::filesystem::path candidates[] = {
                FindGltfValidationAsset("DamagedHelmet", "Assets/models/gltf/DamagedHelmet/DamagedHelmet.gltf"),
                FindGltfValidationAsset("Cube", "Assets/models/gltf/Cube/Cube.gltf")
            };

            for (const std::filesystem::path& path : candidates)
            {
                if (path.empty() || !std::filesystem::exists(path))
                {
                    continue;
                }

                AssetImportResult imported = Assets->ImportAsset(path);
                AssetHandle meshAsset;
                AssetHandle materialAsset;
                if (imported.Succeeded() &&
                    FindFirstMeshAndMaterial(imported, *Assets, meshAsset, materialAsset))
                {
                    CreateValidationSceneEntity(
                        *activeScene,
                        "Stage8A_glTF_Validation",
                        meshAsset,
                        materialAsset);
                    if (const MeshAsset* mesh = Assets->GetMeshAsset(meshAsset))
                    {
                        FrameCameraForMesh(*Scenes, *mesh);
                    }
                    XENGINE_LOG_INFO(
                        std::string("Stage 8A using validation model: ") + path.generic_string());
                    return;
                }

                XENGINE_LOG_WARN(
                    std::string("Stage 8A glTF validation import failed: ") + imported.Diagnostics);
            }

            const AssetHandle meshAsset = Assets->CreateProceduralCubeMeshAsset("Stage8A_ProceduralCube");
            const AssetHandle materialAsset = Assets->CreateTestMaterialAsset(
                "Stage8A_FallbackMaterial",
                baseColorTextureAsset);
            if (meshAsset.IsValid() && materialAsset.IsValid())
            {
                CreateValidationSceneEntity(
                    *activeScene,
                    "Stage8A_ProceduralFallback",
                    meshAsset,
                    materialAsset);
                if (const MeshAsset* mesh = Assets->GetMeshAsset(meshAsset))
                {
                    FrameCameraForMesh(*Scenes, *mesh);
                }
                XENGINE_LOG_INFO("Stage 8A falling back to procedural cube.");
            }

            activeScene->UpdateTransforms();
        }

        void Render(float deltaTime)
        {
            if (!Initialized || RHI == nullptr || !ActivePipeline)
            {
                return;
            }

            RHIDevice* device = RHI->GetDevice();
            if (device == nullptr || !device->IsValid())
            {
                return;
            }

            RHICommandList* commandList = device->BeginFrame();
            if (commandList == nullptr)
            {
                return;
            }

            Scene* activeScene = Scenes != nullptr ? Scenes->GetActiveScene() : nullptr;
            if (activeScene != nullptr && Assets != nullptr)
            {
                RenderExtraction::Extract(*activeScene, *Assets, Resources, SceneData);
            }
            else
            {
                SceneData.Clear();
            }

            RenderFrameContext frame;
            frame.Device = device;
            frame.CommandList = commandList;
            frame.SwapchainWidth = SwapchainWidth;
            frame.SwapchainHeight = SwapchainHeight;
            frame.DeltaTime = deltaTime;

            if (EngineInstance != nullptr)
            {
                const Time& time = EngineInstance->GetTime();
                frame.FrameIndex = static_cast<u32>(time.GetFrameIndex());
                frame.TimeSeconds = time.GetTotalTime();
            }

            const CameraComponent* camera = Scenes != nullptr ? Scenes->GetPrimaryCamera() : nullptr;
            const TransformComponent* cameraTransform =
                Scenes != nullptr ? Scenes->GetPrimaryCameraTransform() : nullptr;
            if (camera != nullptr && cameraTransform != nullptr)
            {
                const float aspect = SwapchainHeight > 0 ?
                    static_cast<float>(SwapchainWidth) / static_cast<float>(SwapchainHeight) :
                    1.0f;
                frame.ViewMatrix = Math::BuildViewMatrixLH_XForward(
                    cameraTransform->GetWorldPosition(),
                    cameraTransform->GetWorldRotation());
                frame.CameraWorldPosition = cameraTransform->GetWorldPosition();

                Mat4 projection;
                if (camera->ProjectionMode == CameraProjectionMode::Orthographic)
                {
                    const float halfHeight = camera->OrthographicHeight * 0.5f;
                    const float halfWidth = halfHeight * aspect;
                    projection = Math::OrthographicLH_ZO(
                        -halfWidth,
                        halfWidth,
                        -halfHeight,
                        halfHeight,
                        camera->NearPlane,
                        camera->FarPlane);
                }
                else
                {
                    projection = Math::PerspectiveLH_ZO(
                        camera->VerticalFovRadians,
                        aspect,
                        camera->NearPlane,
                        camera->FarPlane);
                }

                frame.ProjectionMatrix = ApplyRHIClipSpaceConvention(
                    projection,
                    device->GetClipSpaceConvention());
                frame.ViewProjectionMatrix = frame.ProjectionMatrix * frame.ViewMatrix;
            }
            else
            {
                frame.CameraWorldPosition = Vec3 { -4.0f, 0.0f, 1.5f };
                frame.ViewProjectionMatrix = FallbackViewProjection;
            }

            // Upload per-frame shader data after extraction so lighting reflects
            // the current RenderScene.
            Resources.FrameResources->Update(frame, SceneData);

            ActivePipeline->Render(frame, SceneData, Resources);
            device->EndFrame();
        }
    };

    RenderSystem::RenderSystem()
        : m_Impl(std::make_unique<Impl>())
    {
    }

    RenderSystem::~RenderSystem()
    {
        OnDestroy();
    }

    void RenderSystem::OnCreate(const SubsystemContext& context)
    {
        XENGINE_LOG_INFO("Creating RenderSystem");
        XENGINE_ASSERT(context.Engine != nullptr, "RenderSystem requires a valid Engine");
        if (context.Engine == nullptr)
        {
            return;
        }

        Impl& impl = *m_Impl;
        impl.EngineInstance = context.Engine;

        SubsystemManager& subsystems = context.Engine->GetSubsystemManager();
        impl.RHI = subsystems.GetSubsystem<RHISystem>();
        impl.Shader = subsystems.GetSubsystem<ShaderSystem>();
        impl.Assets = subsystems.GetSubsystem<AssetSystem>();
        impl.Scenes = subsystems.GetSubsystem<SceneSystem>();

        RHIDevice* device = impl.RHI != nullptr ? impl.RHI->GetDevice() : nullptr;
        if (device == nullptr || !device->IsValid() || impl.Shader == nullptr ||
            !impl.Shader->IsCompilerAvailable())
        {
            XENGINE_LOG_ERROR("RenderSystem requires RHIDevice and ShaderSystem");
            return;
        }

        if (context.Config != nullptr)
        {
            impl.SwapchainWidth = context.Config->WindowWidth;
            impl.SwapchainHeight = context.Config->WindowHeight;
        }

        impl.Textures = std::make_unique<RenderTextureManager>();
        impl.Textures->Initialize(device);
        impl.Meshes = std::make_unique<RenderMeshManager>();
        impl.Meshes->Initialize(device);
        impl.Materials = std::make_unique<RenderMaterialSystem>();
        impl.Materials->Initialize(impl.Textures.get(), device);
        impl.Shaders = std::make_unique<RenderShaderLibrary>();
        if (!impl.Shaders->Initialize(device, impl.Shader))
        {
            impl.Shutdown();
            return;
        }
        impl.FrameResources = std::make_unique<RenderFrameResources>();
        if (!impl.FrameResources->Initialize(device))
        {
            impl.Shutdown();
            return;
        }
        impl.PipelineStates = std::make_unique<RenderPipelineStateCache>();
        if (!impl.PipelineStates->Initialize(
            device,
            impl.Shaders.get(),
            impl.Materials.get(),
            impl.FrameResources.get()))
        {
            impl.Shutdown();
            return;
        }

        impl.Resources.Textures = impl.Textures.get();
        impl.Resources.Meshes = impl.Meshes.get();
        impl.Resources.Materials = impl.Materials.get();
        impl.Resources.Shaders = impl.Shaders.get();
        impl.Resources.PipelineStates = impl.PipelineStates.get();
        impl.Resources.FrameResources = impl.FrameResources.get();

        impl.ActivePipeline = std::make_unique<ForwardRenderPipeline>();
        if (!impl.ActivePipeline->Initialize(impl.Resources))
        {
            impl.Shutdown();
            return;
        }

        const float aspect = impl.SwapchainHeight > 0 ?
            static_cast<float>(impl.SwapchainWidth) / static_cast<float>(impl.SwapchainHeight) :
            1.0f;
        const Mat4 projection = ApplyRHIClipSpaceConvention(
            Math::PerspectiveLH_ZO(1.04719755f, aspect, 0.1f, 100.0f),
            device->GetClipSpaceConvention());
        const Mat4 view = Math::LookAtLH_XForward(
            Vec3 { -4.0f, 0.0f, 1.5f },
            Vec3 { 0.0f, 0.0f, 0.0f },
            CoordinateSystem::Up);
        impl.FallbackViewProjection = projection * view;

        impl.CreateValidationScene();
        impl.Initialized = true;
    }

    void RenderSystem::OnDestroy()
    {
        if (m_Impl && m_Impl->Initialized)
        {
            XENGINE_LOG_INFO("Destroying RenderSystem");
        }
        if (m_Impl)
        {
            m_Impl->Shutdown();
        }
    }

    void RenderSystem::OnUpdate(float deltaTime)
    {
        if (m_Impl)
        {
            m_Impl->Render(deltaTime);
        }
    }
}
