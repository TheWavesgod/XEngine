#include "ImGuiLayer.h"

#include <XEngine/Engine/Time.h>
#include <XEngine/Input/InputTypes.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/PlatformEvents.h>
#include <XEngine/Platform/Window.h>
#include <XEngine/Scene/Scene.h>

#include "../Panels/ViewportPanel.h"
#include "../Panels/InspectorPanel.h"
#include "../Panels/MainMenuBar.h"
#include "../Panels/RendererDebugPanel.h"
#include "../Panels/SceneHierarchyPanel.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <string>

namespace XEngine
{
    namespace
    {
        ImGuiKey ToImGuiKey(KeyCode key)
        {
            switch (key)
            {
            case KeyCode::W:
                return ImGuiKey_W;
            case KeyCode::A:
                return ImGuiKey_A;
            case KeyCode::S:
                return ImGuiKey_S;
            case KeyCode::D:
                return ImGuiKey_D;
            case KeyCode::Q:
                return ImGuiKey_Q;
            case KeyCode::E:
                return ImGuiKey_E;
            case KeyCode::Escape:
                return ImGuiKey_Escape;
            case KeyCode::LeftShift:
                return ImGuiKey_LeftShift;
            case KeyCode::RightShift:
                return ImGuiKey_RightShift;
            default:
                return ImGuiKey_None;
            }
        }

        int ToImGuiMouseButton(MouseButton button)
        {
            switch (button)
            {
            case MouseButton::Left:
                return 0;
            case MouseButton::Right:
                return 1;
            case MouseButton::Middle:
                return 2;
            default:
                return 0;
            }
        }
    }

    bool ImGuiLayer::Initialize(Window& window, RHIDevice& device)
    {
        if (m_Initialized)
        {
            return true;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeHandle().Window);
        if (sdlWindow == nullptr || !ImGui_ImplSDL3_InitForVulkan(sdlWindow))
        {
            XENGINE_LOG_ERROR("Failed to initialize ImGui SDL3 backend");
            ImGui::DestroyContext();
            return false;
        }

        if (!m_VulkanBackend.Initialize(device))
        {
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        // ImGui context and frame lifecycle are editor-owned and must not leak
        // into Runtime public APIs.
        m_Initialized = true;
        XENGINE_LOG_INFO("ImGuiLayer initialized");
        return true;
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_VulkanBackend.Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_Initialized = false;
    }

    void ImGuiLayer::HandleEvent(const PlatformEvent& event)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        switch (event.Type)
        {
        case PlatformEventType::KeyDown:
        case PlatformEventType::KeyUp:
        {
            const ImGuiKey key = ToImGuiKey(event.Key);
            if (key != ImGuiKey_None)
            {
                io.AddKeyEvent(key, event.Type == PlatformEventType::KeyDown);
            }
            break;
        }
        case PlatformEventType::MouseButtonDown:
        case PlatformEventType::MouseButtonUp:
            io.AddMouseButtonEvent(
                ToImGuiMouseButton(event.Button),
                event.Type == PlatformEventType::MouseButtonDown);
            break;
        case PlatformEventType::MouseMove:
            io.AddMousePosEvent(event.MouseX, event.MouseY);
            break;
        case PlatformEventType::MouseWheel:
            io.AddMouseWheelEvent(event.WheelDeltaX, event.WheelDeltaY);
            break;
        default:
            break;
        }
    }

    void ImGuiLayer::RenderEditor(
        EditorContext& context,
        const Time& time,
        MainMenuBar& mainMenuBar,
        SceneHierarchyPanel& sceneHierarchyPanel,
        InspectorPanel& inspectorPanel,
        RendererDebugPanel& rendererDebugPanel,
        ViewportPanel& viewportPanel)
    {
        if (!m_Initialized)
        {
            return;
        }

        BeginFrame();
        mainMenuBar.Draw(context);
        DrawDockspace();
        sceneHierarchyPanel.Draw(context);
        inspectorPanel.Draw(context);
        rendererDebugPanel.Draw(context);
        viewportPanel.Draw(context);
        RenderDebugWindow(context, time);
        EndFrame();
    }

    void ImGuiLayer::DrawDockspace()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("Editor Dockspace", nullptr, windowFlags);
        ImGui::PopStyleVar(2);

        const ImGuiID dockspaceId = ImGui::GetID("XEngineEditorDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void ImGuiLayer::RenderDebugWindow(EditorContext& context, const Time& time)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui::Begin("XEngine Editor Debug");
        const float deltaSeconds = time.GetDeltaTime();
        const float fps = deltaSeconds > 0.0f ? 1.0f / deltaSeconds : 0.0f;
        ImGui::Text("Frame time: %.3f ms", deltaSeconds * 1000.0f);
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Current scene: %s", context.CurrentScenePath.empty() ?
            "<none>" :
            context.CurrentScenePath.generic_string().c_str());
        ImGui::Text("Scene dirty: %s", context.SceneDirty ? "true" : "false");
        ImGui::Text("Selected entity: %s", context.SelectedEntity.IsValid() ? "valid" : "invalid");
        ImGui::Text("WantCaptureMouse: %s", WantCaptureMouse() ? "true" : "false");
        ImGui::Text("WantCaptureKeyboard: %s", WantCaptureKeyboard() ? "true" : "false");
        ImGui::Text(
            "Viewport input: %s",
            context.ViewportInputMode == ViewportInputMode::CameraCapture ? "CameraCapture" : "UI");
        ImGui::Text("Viewport hovered: %s", context.ViewportHovered ? "true" : "false");
        ImGui::Text("Viewport focused: %s", context.ViewportFocused ? "true" : "false");
        ImGui::End();
    }

    bool ImGuiLayer::WantCaptureMouse() const
    {
        return m_Initialized && ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGuiLayer::WantCaptureKeyboard() const
    {
        return m_Initialized && ImGui::GetIO().WantCaptureKeyboard;
    }

    void ImGuiLayer::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame()
    {
        ImGui::Render();
        m_VulkanBackend.RenderDrawData();
    }
}
