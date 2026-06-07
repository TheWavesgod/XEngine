#include <XEngine/Renderer/RenderSystem.h>

#include "Materials/MaterialSystem.h"
#include "Passes/ClearPass.h"
#include "Passes/ForwardOpaquePass.h"
#include "Passes/PresentPass.h"
#include "Passes/TrianglePass.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphContext.h"
#include "Resources/RenderMeshManager.h"
#include "Resources/TextureManager.h"
#include "Scene/RenderExtraction.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/Vector.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/Scene/Scene.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderSystem.h>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace XEngine
{
    namespace
    {
        constexpr f32 Pi = 3.14159265358979323846f;

        Matrix4 Identity()
        {
            Matrix4 result {};
            result.Values[0] = 1.0f;
            result.Values[5] = 1.0f;
            result.Values[10] = 1.0f;
            result.Values[15] = 1.0f;
            return result;
        }

        Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs)
        {
            Matrix4 result {};
            for (u32 row = 0; row < 4; ++row)
            {
                for (u32 column = 0; column < 4; ++column)
                {
                    result.Values[column * 4 + row] =
                        lhs.Values[0 * 4 + row] * rhs.Values[column * 4 + 0] +
                        lhs.Values[1 * 4 + row] * rhs.Values[column * 4 + 1] +
                        lhs.Values[2 * 4 + row] * rhs.Values[column * 4 + 2] +
                        lhs.Values[3 * 4 + row] * rhs.Values[column * 4 + 3];
                }
            }
            return result;
        }

        Vector3 Subtract(const Vector3& lhs, const Vector3& rhs)
        {
            return { lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
        }

        Vector3 Cross(const Vector3& lhs, const Vector3& rhs)
        {
            return {
                lhs.Y * rhs.Z - lhs.Z * rhs.Y,
                lhs.Z * rhs.X - lhs.X * rhs.Z,
                lhs.X * rhs.Y - lhs.Y * rhs.X
            };
        }

        f32 Dot(const Vector3& lhs, const Vector3& rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
        }

        Vector3 Normalize(const Vector3& value)
        {
            const f32 length = std::sqrt(Dot(value, value));
            if (length <= 0.0f)
            {
                return {};
            }
            return { value.X / length, value.Y / length, value.Z / length };
        }

        Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
        {
            const Vector3 forward = Normalize(Subtract(target, eye));
            const Vector3 right = Normalize(Cross(forward, up));
            const Vector3 cameraUp = Cross(right, forward);

            Matrix4 result = Identity();
            result.Values[0] = right.X;
            result.Values[1] = cameraUp.X;
            result.Values[2] = -forward.X;
            result.Values[4] = right.Y;
            result.Values[5] = cameraUp.Y;
            result.Values[6] = -forward.Y;
            result.Values[8] = right.Z;
            result.Values[9] = cameraUp.Z;
            result.Values[10] = -forward.Z;
            result.Values[12] = -Dot(right, eye);
            result.Values[13] = -Dot(cameraUp, eye);
            result.Values[14] = Dot(forward, eye);
            return result;
        }

        Matrix4 Perspective(f32 fovRadians, f32 aspect, f32 nearPlane, f32 farPlane)
        {
            const f32 tanHalfFov = std::tan(fovRadians * 0.5f);
            Matrix4 result {};
            result.Values[0] = 1.0f / (aspect * tanHalfFov);
            result.Values[5] = -1.0f / tanHalfFov;
            result.Values[10] = farPlane / (nearPlane - farPlane);
            result.Values[11] = -1.0f;
            result.Values[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);
            return result;
        }

        RHIShaderDesc MakeRHIShaderDesc(const CompiledShader& shader, const char* debugName)
        {
            RHIShaderDesc desc;
            desc.Stage = shader.Stage;
            desc.Target = shader.Target;
            desc.Format = shader.Format;
            desc.EntryPoint = "main";
            desc.Code = shader.Bytecode.data();
            desc.CodeSize = shader.Bytecode.size();
            desc.DebugName = debugName;
            return desc;
        }

        struct PBRPipelinePushConstants
        {
            Matrix4 ModelViewProjection;
            Vec4 BaseColorFactor;
            Vec4 MaterialFactors;
        };

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

        Entity CreateValidationSceneEntity(
            Scene& scene,
            const char* name,
            AssetHandle meshAsset,
            AssetHandle materialAsset)
        {
            Entity entity = scene.CreateEntity(name != nullptr ? name : "Renderable");
            TransformComponent& transform = scene.AddTransform(entity);
            transform.Dirty = true;

            MeshRendererComponent& renderer = scene.AddMeshRenderer(entity);
            renderer.MeshAsset = meshAsset;
            renderer.MaterialAsset = materialAsset;
            return entity;
        }

        std::filesystem::path FindGltfValidationAsset(const char* preferredName, const std::filesystem::path& fallback)
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

            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root))
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
            const float radius = std::max(glm::length(meshAsset.Bounds.GetExtents()), 0.5f);
            sceneSystem.FrameDebugCamera(center, radius);
        }
    }

    RenderSystem::RenderSystem() = default;

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
            XENGINE_LOG_ERROR("RenderSystem requires a valid Engine");
            return;
        }

        m_RHISystem = context.Engine->GetSubsystemManager().GetSubsystem<RHISystem>();
        XENGINE_ASSERT(m_RHISystem != nullptr, "RenderSystem requires RHISystem");
        if (m_RHISystem == nullptr)
        {
            XENGINE_LOG_ERROR("RenderSystem requires RHISystem");
            return;
        }

        ShaderSystem* shaderSystem = context.Engine->GetSubsystemManager().GetSubsystem<ShaderSystem>();
        XENGINE_ASSERT(shaderSystem != nullptr, "RenderSystem requires ShaderSystem for Stage 4B");
        if (shaderSystem == nullptr || !shaderSystem->IsCompilerAvailable())
        {
            XENGINE_LOG_ERROR("RenderSystem requires an available ShaderSystem");
            return;
        }

        RHIDevice* device = m_RHISystem->GetDevice();
        XENGINE_ASSERT(device != nullptr, "RenderSystem requires a valid RHIDevice");
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderSystem requires a valid RHIDevice");
            return;
        }

        m_TextureManager = std::make_unique<TextureManager>();
        m_TextureManager->Initialize(device);

        AssetHandle baseColorTextureAssetHandle;
        m_AssetSystem = context.Engine->GetSubsystemManager().GetSubsystem<AssetSystem>();
        m_SceneSystem = context.Engine->GetSubsystemManager().GetSubsystem<SceneSystem>();
        AssetSystem* assetSystem = m_AssetSystem;
        const std::string checkerPath = "Assets/Textures/checker.jpg";
        if (std::filesystem::exists(checkerPath))
        {
            if (assetSystem != nullptr)
            {
                AssetImportResult importResult = assetSystem->ImportAsset(checkerPath);
                const TextureAsset* textureAsset = importResult.Succeeded() ?
                    assetSystem->GetTextureAsset(importResult.MainAsset) :
                    nullptr;
                if (textureAsset != nullptr)
                {
                    baseColorTextureAssetHandle = importResult.MainAsset;
                }
                else
                {
                    XENGINE_LOG_WARN("AssetSystem could not import checker texture; using default texture.");
                }
            }
            else
            {
                XENGINE_LOG_WARN("AssetSystem is unavailable; using default texture validation only.");
            }
        }
        else
        {
            XENGINE_LOG_WARN("Assets/Textures/checker.jpg not found; using default texture validation only.");
        }

        m_MaterialSystem = std::make_unique<MaterialSystem>();
        m_MaterialSystem->Initialize(m_TextureManager.get(), device);

        m_RenderMeshManager = std::make_unique<RenderMeshManager>();
        m_RenderMeshManager->Initialize(device);

        bool validationSceneCreated = false;
        Scene* activeScene = m_SceneSystem != nullptr ? m_SceneSystem->GetActiveScene() : nullptr;
        if (assetSystem != nullptr && activeScene != nullptr)
        {
            const std::filesystem::path gltfCandidates[] = {
                FindGltfValidationAsset("DamagedHelmet", "Assets/models/gltf/DamagedHelmet/DamagedHelmet.gltf"),
                FindGltfValidationAsset("Cube", "Assets/models/gltf/Cube/Cube.gltf")
            };

            for (const std::filesystem::path& gltfValidationPath : gltfCandidates)
            {
                if (gltfValidationPath.empty())
                {
                    continue;
                }

                if (!std::filesystem::exists(gltfValidationPath))
                {
                    continue;
                }

                AssetImportResult gltfImport = assetSystem->ImportAsset(gltfValidationPath);
                if (gltfImport.Succeeded())
                {
                    AssetHandle importedMeshAsset;
                    AssetHandle importedMaterialAsset;
                    if (FindFirstMeshAndMaterial(gltfImport, *assetSystem, importedMeshAsset, importedMaterialAsset))
                    {
                        CreateValidationSceneEntity(
                            *activeScene,
                            "Stage7G_glTF_Validation",
                            importedMeshAsset,
                            importedMaterialAsset);

                        if (const MeshAsset* meshAsset = assetSystem->GetMeshAsset(importedMeshAsset))
                        {
                            FrameCameraForMesh(*m_SceneSystem, *meshAsset);
                        }

                        XENGINE_LOG_INFO(
                            std::string("Stage 7G using validation model: ") +
                            gltfValidationPath.generic_string());
                        validationSceneCreated = true;
                        break;
                    }
                }
                else
                {
                    XENGINE_LOG_WARN(
                        std::string("Stage 7G glTF validation import failed: ") +
                        gltfImport.Diagnostics);
                }
            }

            if (!validationSceneCreated)
            {
                const AssetHandle meshAsset = assetSystem->CreateProceduralCubeMeshAsset("Stage7F_ProceduralCube");
                const AssetHandle materialAsset = assetSystem->CreateTestMaterialAsset(
                    "Stage7F_FallbackMaterial",
                    baseColorTextureAssetHandle);
                if (meshAsset.IsValid() && materialAsset.IsValid())
                {
                    CreateValidationSceneEntity(*activeScene, "Stage7G_ProceduralFallback", meshAsset, materialAsset);
                    if (const MeshAsset* mesh = assetSystem->GetMeshAsset(meshAsset))
                    {
                        FrameCameraForMesh(*m_SceneSystem, *mesh);
                    }
                    validationSceneCreated = true;
                    XENGINE_LOG_INFO("Stage 7G falling back to procedural cube.");
                }
            }
        }
        else
        {
            XENGINE_LOG_WARN("Stage 7G validation Scene setup skipped because AssetSystem or SceneSystem is unavailable.");
        }

        ShaderCompileDesc vertexDesc;
        vertexDesc.Path = "Engine/Shaders/Passes/ForwardPBR.slang";
        vertexDesc.EntryPoint = "vertexMain";
        vertexDesc.Stage = ShaderStage::Vertex;
        vertexDesc.Target = ShaderTarget::VulkanSPIRV;
        vertexDesc.GenerateDebugInfo = true;
        vertexDesc.EnableOptimization = false;

        XENGINE_LOG_INFO("Compiling ForwardPBR.slang vertexMain");
        CompiledShader vertexShader = shaderSystem->Compile(vertexDesc);
        if (!vertexShader.IsValid())
        {
            XENGINE_LOG_ERROR(vertexShader.Diagnostics.empty() ? "ForwardPBR vertex shader compilation failed" :
                                                                  vertexShader.Diagnostics);
            return;
        }

        if (context.Config != nullptr && context.Config->WindowHeight > 0)
        {
            m_AspectRatio =
                static_cast<float>(context.Config->WindowWidth) /
                static_cast<float>(context.Config->WindowHeight);
        }

        ShaderCompileDesc fragmentDesc;
        fragmentDesc.Path = "Engine/Shaders/Passes/ForwardPBR.slang";
        fragmentDesc.EntryPoint = "fragmentMain";
        fragmentDesc.Stage = ShaderStage::Fragment;
        fragmentDesc.Target = ShaderTarget::VulkanSPIRV;
        fragmentDesc.GenerateDebugInfo = true;
        fragmentDesc.EnableOptimization = false;

        XENGINE_LOG_INFO("Compiling ForwardPBR.slang fragmentMain");
        CompiledShader fragmentShader = shaderSystem->Compile(fragmentDesc);
        if (!fragmentShader.IsValid())
        {
            XENGINE_LOG_ERROR(fragmentShader.Diagnostics.empty() ? "ForwardPBR fragment shader compilation failed" :
                                                                    fragmentShader.Diagnostics);
            return;
        }

        m_MeshVertexShader = device->CreateShader(MakeRHIShaderDesc(vertexShader, "ForwardPBR vertex"));
        if (!m_MeshVertexShader)
        {
            XENGINE_LOG_ERROR("Failed to create ForwardPBR vertex RHI shader");
            return;
        }

        m_MeshFragmentShader = device->CreateShader(MakeRHIShaderDesc(fragmentShader, "ForwardPBR fragment"));
        if (!m_MeshFragmentShader)
        {
            XENGINE_LOG_ERROR("Failed to create ForwardPBR fragment RHI shader");
            return;
        }

        RHIGraphicsPipelineDesc pipelineDesc;
        pipelineDesc.VertexShader = m_MeshVertexShader.get();
        pipelineDesc.FragmentShader = m_MeshFragmentShader.get();
        pipelineDesc.ColorFormat = device->GetSwapchainFormat();
        pipelineDesc.DepthFormat = RHIFormat::D32Float;
        pipelineDesc.EnableDepthTest = true;
        pipelineDesc.EnableDepthWrite = true;
        pipelineDesc.VertexLayout.Stride = sizeof(MeshVertex);
        pipelineDesc.VertexLayout.Attributes = {
            RHIVertexAttributeDesc { 0, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Position)) },
            RHIVertexAttributeDesc { 1, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Normal)) },
            RHIVertexAttributeDesc { 2, RHIFormat::R32G32Float, static_cast<u32>(offsetof(MeshVertex, TexCoord0)) }
        };
        pipelineDesc.BindGroupLayouts.push_back(m_MaterialSystem->GetPBRMaterialBindGroupLayout());
        pipelineDesc.PushConstantSize = sizeof(PBRPipelinePushConstants);
        pipelineDesc.PushConstantStages = RHIShaderStageFlags::AllGraphics;
        pipelineDesc.DebugName = "Creating PBR graphics pipeline";

        m_MeshPipeline = device->CreateGraphicsPipeline(pipelineDesc);
        if (!m_MeshPipeline)
        {
            XENGINE_LOG_ERROR("Failed to create ForwardPBR graphics pipeline");
            return;
        }

        Mat4 projection = glm::perspective(60.0f * Pi / 180.0f, m_AspectRatio, 0.1f, 100.0f);
        projection[1][1] *= -1.0f;
        const Mat4 view = glm::lookAt(
            Vec3 { 0.0f, 1.5f, 4.0f },
            Vec3 { 0.0f, 0.0f, 0.0f },
            Vec3 { 0.0f, 1.0f, 0.0f });
        // Fallback camera used only if Scene has no primary camera.
        m_ViewProjection = projection * view;

        m_Initialized = true;
    }

    void RenderSystem::OnDestroy()
    {
        if (m_Initialized)
        {
            XENGINE_LOG_INFO("Destroying RenderSystem");
        }

        if (m_RHISystem != nullptr)
        {
            RHIDevice* device = m_RHISystem->GetDevice();
            if (device != nullptr && device->IsValid())
            {
                device->WaitIdle();
            }
        }

        m_MeshPipeline.reset();
        m_MeshFragmentShader.reset();
        m_MeshVertexShader.reset();
        m_RenderScene.Clear();
        if (m_RenderMeshManager)
        {
            m_RenderMeshManager->Shutdown();
            m_RenderMeshManager.reset();
        }
        if (m_MaterialSystem)
        {
            m_MaterialSystem->Shutdown();
            m_MaterialSystem.reset();
        }
        if (m_TextureManager)
        {
            m_TextureManager->Shutdown();
            m_TextureManager.reset();
        }
        m_AssetSystem = nullptr;
        m_SceneSystem = nullptr;
        m_RHISystem = nullptr;
        m_Initialized = false;
    }

    void RenderSystem::OnUpdate(float deltaTime)
    {
        (void)deltaTime;
        Render();
    }

    void RenderSystem::Render()
    {
        if (m_RHISystem == nullptr)
        {
            return;
        }

        RHIDevice* device = m_RHISystem->GetDevice();
        if (device == nullptr || !device->IsValid())
        {
            return;
        }

        RHICommandList* commandList = device->BeginFrame();

        RHIColor clearColor;
        clearColor.R = 0.1f;
        clearColor.G = 0.1f;
        clearColor.B = 0.15f;
        clearColor.A = 1.0f;

        RenderGraph graph;
        graph.Clear();
        AddClearPass(graph, clearColor);

        Scene* activeScene = m_SceneSystem != nullptr ? m_SceneSystem->GetActiveScene() : nullptr;
        if (activeScene != nullptr && m_AssetSystem != nullptr && m_RenderMeshManager && m_MaterialSystem && m_TextureManager)
        {
            RenderExtraction::Extract(
                *activeScene,
                *m_AssetSystem,
                *m_RenderMeshManager,
                *m_MaterialSystem,
                *m_TextureManager,
                m_RenderScene);
        }
        else
        {
            m_RenderScene.Clear();
        }

        Mat4 viewProjection = m_ViewProjection;
        const CameraComponent* primaryCamera = m_SceneSystem != nullptr ? m_SceneSystem->GetPrimaryCamera() : nullptr;
        const TransformComponent* primaryCameraTransform =
            m_SceneSystem != nullptr ? m_SceneSystem->GetPrimaryCameraTransform() : nullptr;
        if (primaryCamera != nullptr && primaryCameraTransform != nullptr)
        {
            viewProjection =
                BuildCameraProjectionMatrix(*primaryCamera, m_AspectRatio) *
                BuildCameraViewMatrix(*primaryCameraTransform);
        }

        AddForwardOpaquePass(
            graph,
            m_MeshPipeline.get(),
            m_MaterialSystem.get(),
            m_RenderMeshManager.get(),
            m_RenderScene,
            viewProjection);
        AddPresentPass(graph);
        graph.Compile();

        RenderGraphContext context(*device, commandList);
        graph.Execute(context);

        device->EndFrame();
    }
}
