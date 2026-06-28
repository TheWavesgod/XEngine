#include <XEngine/Editor/EditorSystem.h>

#include "FreeCameraController.h"
#include "ImGui/ImGuiLayer.h"
#include "Panels/InspectorPanel.h"
#include "Panels/MainMenuBar.h"
#include "Panels/RendererDebugPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ViewportPanel.h"
#include "Viewport/EditorViewportRenderTarget.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Core/ProjectPaths.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/PlatformEvents.h>
#include <XEngine/Platform/PlatformSystem.h>
#include <XEngine/Platform/Window.h>
#include <XEngine/Renderer/RenderSystem.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/Scene/SceneSerializer.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Serialization/JsonSerialization.h>
#include <XEngine/Serialization/SerializationContext.h>

#include <algorithm>

namespace XEngine
{
    namespace
    {
        void LoadDefaultEditorSettings(EditorContext& context)
        {
            JsonSerialization::Json settings;
            if (!JsonSerialization::LoadJsonFile(
                    ProjectPaths::Resolve("config://Editor/DefaultEditorSettings.json"),
                    settings))
            {
                return;
            }

            context.CurrentScenePath = settings.value(
                "startupScene",
                context.CurrentScenePath.generic_string());
            context.UseEditorCamera = settings.value("useEditorCamera", context.UseEditorCamera);
            context.ShowSceneHierarchy = settings.value("showSceneHierarchy", context.ShowSceneHierarchy);
            context.ShowInspector = settings.value("showInspector", context.ShowInspector);
            context.ShowRendererDebug = settings.value("showRendererDebug", context.ShowRendererDebug);
        }
    }

    EditorSystem::EditorSystem() = default;

    EditorSystem::~EditorSystem()
    {
        OnDestroy();
    }

    void EditorSystem::OnCreate(const SubsystemContext& context)
    {
        m_Engine = context.Engine;
        if (m_Engine == nullptr)
        {
            return;
        }

        SubsystemManager& subsystems = m_Engine->GetSubsystemManager();
        PlatformSystem* platformSystem = subsystems.GetSubsystem<PlatformSystem>();
        RHISystem* rhiSystem = subsystems.GetSubsystem<RHISystem>();
        RenderSystem* renderSystem = subsystems.GetSubsystem<RenderSystem>();
        SceneSystem* sceneSystem = subsystems.GetSubsystem<SceneSystem>();
        AssetSystem* assetSystem = subsystems.GetSubsystem<AssetSystem>();

        Window* window = platformSystem != nullptr ? platformSystem->GetMainWindow() : nullptr;
        RHIDevice* device = rhiSystem != nullptr ? rhiSystem->GetDevice() : nullptr;
        if (window == nullptr || device == nullptr || renderSystem == nullptr)
        {
            XENGINE_LOG_ERROR("EditorSystem requires PlatformSystem, RHISystem, and RenderSystem");
            return;
        }

        m_Context.ActiveScene = sceneSystem != nullptr ? sceneSystem->GetActiveScene() : nullptr;
        m_Context.Assets = assetSystem;
        m_Context.RendererDebug = &renderSystem->GetDebugSettings();
        m_Context.CurrentScenePath = "asset://Scenes/Default.xscene";
        LoadDefaultEditorSettings(m_Context);
        m_Window = window;

        if (m_Context.ActiveScene != nullptr)
        {
            SerializationContext serializationContext;
            serializationContext.Assets = m_Context.Assets;

            // Editor startup uses the Runtime SceneSerializer; Editor only
            // selects the fixed stage default scene path.
            SceneSerializer serializer(serializationContext);
            if (serializer.LoadFromFile(*m_Context.ActiveScene, m_Context.CurrentScenePath))
            {
                m_Context.SceneDirty = false;
                m_Context.SelectedEntity = {};
                XENGINE_LOG_INFO(
                    std::string("Loaded editor startup scene: ") +
                    m_Context.CurrentScenePath.generic_string());
            }
            else
            {
                XENGINE_LOG_ERROR(
                    std::string("Failed to load editor startup scene: ") +
                    m_Context.CurrentScenePath.generic_string());
            }
        }

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();
        if (!m_ImGuiLayer->Initialize(*window, *device))
        {
            m_ImGuiLayer.reset();
            return;
        }
        m_MainMenuBar = std::make_unique<MainMenuBar>();
        m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();
        m_InspectorPanel = std::make_unique<InspectorPanel>();
        m_RendererDebugPanel = std::make_unique<RendererDebugPanel>();
        m_ViewportPanel = std::make_unique<ViewportPanel>();
        m_ViewportRenderTarget = std::make_unique<EditorViewportRenderTarget>();
        m_FreeCameraController = std::make_unique<FreeCameraController>();
        UpdateViewportRenderTarget();

        renderSystem->SetViewProvider(
            [this](RenderView& outView)
            {
                if (!m_Context.UseEditorCamera)
                {
                    return false;
                }

                const float aspect = m_Context.ViewportHeight > 0 ?
                    static_cast<float>(m_Context.ViewportWidth) / static_cast<float>(m_Context.ViewportHeight) :
                    1.0f;
                outView = m_Context.Camera.BuildRenderView(aspect);
                return true;
            });

        renderSystem->SetOutputProvider(
            [this](RHIRenderOutputDesc& outOutput)
            {
                if (m_ViewportRenderTarget == nullptr || !m_ViewportRenderTarget->IsValid())
                {
                    return false;
                }

                // Editor renders the scene offscreen so ImGui owns swapchain
                // composition and can place the image inside the Viewport panel.
                outOutput = m_ViewportRenderTarget->BuildRenderOutput();
                return true;
            });

        renderSystem->SetOverlayCallback(
            [this]()
            {
                if (m_Engine != nullptr && m_ImGuiLayer != nullptr && m_MainMenuBar != nullptr &&
                    m_SceneHierarchyPanel != nullptr && m_InspectorPanel != nullptr &&
                    m_RendererDebugPanel != nullptr && m_ViewportPanel != nullptr)
                {
                    m_ImGuiLayer->RenderEditor(
                        m_Context,
                        m_Engine->GetTime(),
                        *m_MainMenuBar,
                        *m_SceneHierarchyPanel,
                        *m_InspectorPanel,
                        *m_RendererDebugPanel,
                        *m_ViewportPanel);
                }
            });

        m_Initialized = true;
        XENGINE_LOG_INFO("EditorSystem initialized");
    }

    void EditorSystem::OnDestroy()
    {
        if (!m_Initialized && !m_ImGuiLayer)
        {
            return;
        }

        if (m_Engine != nullptr)
        {
            if (RenderSystem* renderSystem = m_Engine->GetSubsystemManager().GetSubsystem<RenderSystem>())
            {
                renderSystem->SetOverlayCallback({});
                renderSystem->SetViewProvider({});
                renderSystem->SetOutputProvider({});
            }
        }

        ReleaseViewportCapture();
        if (m_ImGuiLayer != nullptr && m_ViewportTextureId != 0)
        {
            m_ImGuiLayer->UnregisterTexture(static_cast<ImTextureID>(m_ViewportTextureId));
            m_ViewportTextureId = 0;
        }
        m_Context.ViewportTextureId = 0;
        m_ViewportRenderTarget.reset();
        if (m_ImGuiLayer)
        {
            m_ImGuiLayer->Shutdown();
            m_ImGuiLayer.reset();
        }
        m_MainMenuBar.reset();
        m_SceneHierarchyPanel.reset();
        m_InspectorPanel.reset();
        m_RendererDebugPanel.reset();
        m_ViewportPanel.reset();
        m_FreeCameraController.reset();

        m_Context = {};
        m_Window = nullptr;
        m_Engine = nullptr;
        m_Initialized = false;
        XENGINE_LOG_INFO("EditorSystem shutdown");
    }

    void EditorSystem::OnBeginFrame()
    {
        if (!m_Initialized || m_Engine == nullptr || m_ImGuiLayer == nullptr)
        {
            return;
        }

        PlatformSystem* platformSystem = m_Engine->GetSubsystemManager().GetSubsystem<PlatformSystem>();
        if (platformSystem == nullptr)
        {
            return;
        }

        for (const PlatformEvent& event : platformSystem->GetEvents())
        {
            m_ImGuiLayer->HandleEvent(event);
            m_FreeCameraController->ProcessEvent(event);

            if ((event.Type == PlatformEventType::KeyDown && event.Key == KeyCode::Escape) ||
                event.Type == PlatformEventType::WindowFocusLost)
            {
                ReleaseViewportCapture();
            }
        }

        if (m_Window == nullptr || !m_Window->IsFocused() || !m_Context.UseEditorCamera)
        {
            ReleaseViewportCapture();
        }

        m_FreeCameraController->Update(
            m_Context.Camera,
            m_Engine->GetTime().GetDeltaTime(),
            m_Context.ViewportInputMode == ViewportInputMode::CameraCapture);
        ApplyViewportCaptureState();
    }

    void EditorSystem::OnUpdate(float deltaTime)
    {
        (void)deltaTime;
        if (m_Engine == nullptr)
        {
            return;
        }

        if (SceneSystem* sceneSystem = m_Engine->GetSubsystemManager().GetSubsystem<SceneSystem>())
        {
            m_Context.ActiveScene = sceneSystem->GetActiveScene();
        }
        if (AssetSystem* assetSystem = m_Engine->GetSubsystemManager().GetSubsystem<AssetSystem>())
        {
            m_Context.Assets = assetSystem;
        }
        if (RenderSystem* renderSystem = m_Engine->GetSubsystemManager().GetSubsystem<RenderSystem>())
        {
            m_Context.RendererDebug = &renderSystem->GetDebugSettings();
        }
        UpdateViewportRenderTarget();
    }

    void EditorSystem::ReleaseViewportCapture()
    {
        m_Context.ViewportInputMode = ViewportInputMode::UI;
        ApplyViewportCaptureState();
    }

    void EditorSystem::ApplyViewportCaptureState()
    {
        if (m_Window == nullptr)
        {
            return;
        }

        const bool shouldCapture = m_Context.ViewportInputMode == ViewportInputMode::CameraCapture;
        if (shouldCapture == m_CaptureApplied)
        {
            return;
        }

        m_CaptureApplied = shouldCapture;
        m_Window->SetCursorVisible(!shouldCapture);
        m_Window->SetRelativeMouseMode(shouldCapture);
    }

    void EditorSystem::UpdateViewportRenderTarget()
    {
        if (m_Engine == nullptr || m_ImGuiLayer == nullptr || m_ViewportRenderTarget == nullptr)
        {
            return;
        }

        RHISystem* rhiSystem = m_Engine->GetSubsystemManager().GetSubsystem<RHISystem>();
        RHIDevice* device = rhiSystem != nullptr ? rhiSystem->GetDevice() : nullptr;
        if (device == nullptr)
        {
            return;
        }

        const u32 width = std::max(1u, m_Context.ViewportWidth);
        const u32 height = std::max(1u, m_Context.ViewportHeight);
        const bool changed =
            m_ViewportRenderTarget->GetWidth() != width ||
            m_ViewportRenderTarget->GetHeight() != height ||
            !m_ViewportRenderTarget->IsValid();
        if (!changed)
        {
            return;
        }

        device->WaitIdle();
        if (m_ViewportTextureId != 0)
        {
            m_ImGuiLayer->UnregisterTexture(static_cast<ImTextureID>(m_ViewportTextureId));
            m_ViewportTextureId = 0;
            m_Context.ViewportTextureId = 0;
        }

        // Recreate only when the ImGui Viewport content size changes. The size
        // is collected by the panel and applied on the following frame.
        if (!m_ViewportRenderTarget->Resize(*device, width, height))
        {
            return;
        }

        RHITextureView* colorTextureView = m_ViewportRenderTarget->GetColorTextureView();
        RHISampler* sampler = m_ViewportRenderTarget->GetSampler();
        if (colorTextureView == nullptr || sampler == nullptr)
        {
            return;
        }

        m_ViewportTextureId = static_cast<u64>(
            m_ImGuiLayer->RegisterTexture(*sampler, *colorTextureView));
        m_Context.ViewportTextureId = m_ViewportTextureId;
    }
}
