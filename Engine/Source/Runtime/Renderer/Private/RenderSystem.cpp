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
#include "Shadows/RenderShadowManager.h"
#include "Shadows/RenderShadowType.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/CameraMatrices.h>
#include <XEngine/Math/CoordinateSystem.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/Scene/Scene.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Shader/ShaderSystem.h>

#include <memory>
#include <string>

namespace XEngine
{
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
        std::unique_ptr<RenderShadowManager> ShadowManager;
        RenderResourceContext Resources;

        std::unique_ptr<RenderPipeline> ActivePipeline;
        RenderScene SceneData;
        RendererDebugSettings DebugSettings;
        std::function<void()> OverlayCallback;
        std::function<bool(RenderView&)> ViewProvider;
        std::function<bool(RHIRenderOutputDesc&)> OutputProvider;

        RendererSettings m_RendererSettings;

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
            if (ShadowManager)
            {
                ShadowManager->Shutdown();
                ShadowManager.reset();
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
            RHIRenderOutputDesc output;
            output.Viewport = RHIRect2D { 0, 0, SwapchainWidth, SwapchainHeight };
            output.ColorFormat = device->GetSwapchainFormat();
            output.DepthFormat = RHIFormat::D32Float;
            output.RenderToSwapchain = true;
            if (OutputProvider)
            {
                RHIRenderOutputDesc providedOutput;
                if (OutputProvider(providedOutput) &&
                    providedOutput.Viewport.Width > 0 &&
                    providedOutput.Viewport.Height > 0)
                {
                    output = providedOutput;
                }
            }

            commandList->SetRenderOutput(output);
            frame.Output = output;
            frame.SwapchainWidth = output.Viewport.Width;
            frame.SwapchainHeight = output.Viewport.Height;
            frame.DeltaTime = deltaTime;

            if (EngineInstance != nullptr)
            {
                const Time& time = EngineInstance->GetTime();
                frame.FrameIndex = static_cast<u32>(time.GetFrameIndex());
                frame.TimeSeconds = time.GetTotalTime();
            }

            const float aspect = frame.SwapchainHeight > 0 ?
                static_cast<float>(frame.SwapchainWidth) / static_cast<float>(frame.SwapchainHeight) :
                1.0f;

            RenderView renderView;
            bool hasRenderView = false;
            if (ViewProvider)
            {
                hasRenderView = ViewProvider(renderView);
            }

            const CameraComponent* camera = Scenes != nullptr && !hasRenderView ? Scenes->GetPrimaryCamera() : nullptr;
            const TransformComponent* cameraTransform =
                Scenes != nullptr && !hasRenderView ? Scenes->GetPrimaryCameraTransform() : nullptr;
            if (hasRenderView)
            {
                frame.ViewMatrix = renderView.View;
                frame.ProjectionMatrix = ApplyRHIClipSpaceConvention(
                    renderView.Projection,
                    device->GetClipSpaceConvention());
                frame.ViewProjectionMatrix = frame.ProjectionMatrix * frame.ViewMatrix;
                frame.CameraWorldPosition = renderView.Position;
            }
            else if (camera != nullptr && cameraTransform != nullptr)
            {
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
            // the current RenderScene. ShadowManager is prepared first so its
            // Directional.Cascades can be fed through FillGPUShadowData into data.Shadows.
            Resources.ShadowManager->PrepareFrame(
                *device, SceneData, frame,
                m_RendererSettings.Shadows,
                DebugSettings.Shadows);
            Resources.FrameResources->Update(frame, SceneData, *Resources.ShadowManager);

            ActivePipeline->Render(frame, SceneData, Resources);
            if (!output.RenderToSwapchain && output.ColorTargetView != nullptr)
            {
                commandList->TransitionTextureToShaderRead(output.ColorTargetView);
            }
            if (OverlayCallback)
            {
                OverlayCallback();
            }
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
        // FrameResources needs the shadow manager's resources ready so it can bind the
        // shadow sampled view/sampler into Set 0. Initialize ShadowManager first.
        impl.ShadowManager = std::make_unique<RenderShadowManager>();
        impl.ShadowManager->Initialize(*device);

        if (!impl.FrameResources->Initialize(
                device,
                impl.ShadowManager->GetFrameData().Directional.SampledView,
                impl.ShadowManager->GetFrameData().Directional.Sampler))
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
        impl.Resources.ShadowManager = impl.ShadowManager.get();

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

        // Scene creation/loading belongs to Sandbox or editor code; RenderSystem only renders
        // the active Scene after RenderExtraction has converted it to renderer data.
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

    void RenderSystem::SetOverlayCallback(std::function<void()> callback)
    {
        if (m_Impl)
        {
            m_Impl->OverlayCallback = std::move(callback);
        }
    }

    void RenderSystem::SetViewProvider(std::function<bool(RenderView&)> provider)
    {
        if (m_Impl)
        {
            m_Impl->ViewProvider = std::move(provider);
        }
    }

    void RenderSystem::SetOutputProvider(std::function<bool(RHIRenderOutputDesc&)> provider)
    {
        if (m_Impl)
        {
            m_Impl->OutputProvider = std::move(provider);
        }
    }

    RendererDebugSettings& RenderSystem::GetDebugSettings()
    {
        return m_Impl->DebugSettings;
    }

    const RendererDebugSettings& RenderSystem::GetDebugSettings() const
    {
        return m_Impl->DebugSettings;
    }

    RendererSettings& RenderSystem::GetSettings()
    {
        static RendererSettings s_Default;
        return m_Impl ? m_Impl->m_RendererSettings : s_Default;
    }

    const RendererSettings& RenderSystem::GetSettings() const
    {
        static RendererSettings s_Default;
        return m_Impl ? m_Impl->m_RendererSettings : s_Default;
    }
}
