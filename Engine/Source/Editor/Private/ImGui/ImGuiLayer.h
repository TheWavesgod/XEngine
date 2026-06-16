#pragma once

#include "ImGuiVulkanBackend.h"

#include <XEngine/Editor/EditorContext.h>

namespace XEngine
{
    class RHIDevice;
    class InspectorPanel;
    class MainMenuBar;
    class RendererDebugPanel;
    class SceneHierarchyPanel;
    class Time;
    class ViewportPanel;
    class Window;
    struct PlatformEvent;

    class ImGuiLayer
    {
    public:
        bool Initialize(Window& window, RHIDevice& device);
        void Shutdown();

        void HandleEvent(const PlatformEvent& event);
        void RenderEditor(
            EditorContext& context,
            const Time& time,
            MainMenuBar& mainMenuBar,
            SceneHierarchyPanel& sceneHierarchyPanel,
            InspectorPanel& inspectorPanel,
            RendererDebugPanel& rendererDebugPanel,
            ViewportPanel& viewportPanel);

        bool WantCaptureMouse() const;
        bool WantCaptureKeyboard() const;
        bool IsInitialized() const { return m_Initialized; }

    private:
        void BeginFrame();
        void EndFrame();
        void DrawDockspace();
        void RenderDebugWindow(EditorContext& context, const Time& time);

        ImGuiVulkanBackend m_VulkanBackend;
        bool m_Initialized = false;
    };
}
