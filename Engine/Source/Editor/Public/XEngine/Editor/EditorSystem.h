#pragma once

#include <XEngine/Editor/EditorContext.h>
#include <XEngine/Engine/Subsystem.h>

#include <memory>

namespace XEngine
{
    class FreeCameraController;
    class EditorViewportRenderTarget;
    class ImGuiLayer;
    class InspectorPanel;
    class MainMenuBar;
    class RendererDebugPanel;
    class SceneHierarchyPanel;
    class ViewportPanel;
    class Window;

    class EditorSystem : public ISubsystem
    {
    public:
        EditorSystem();
        ~EditorSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnBeginFrame() override;
        void OnUpdate(float deltaTime) override;

    private:
        void ReleaseViewportCapture();
        void ApplyViewportCaptureState();
        void UpdateViewportRenderTarget();

        EditorContext m_Context;
        Engine* m_Engine = nullptr;
        Window* m_Window = nullptr;
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
        std::unique_ptr<MainMenuBar> m_MainMenuBar;
        std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::unique_ptr<InspectorPanel> m_InspectorPanel;
        std::unique_ptr<RendererDebugPanel> m_RendererDebugPanel;
        std::unique_ptr<ViewportPanel> m_ViewportPanel;
        std::unique_ptr<EditorViewportRenderTarget> m_ViewportRenderTarget;
        std::unique_ptr<FreeCameraController> m_FreeCameraController;
        u64 m_ViewportTextureId = 0;
        bool m_CaptureApplied = false;
        bool m_Initialized = false;
    };
}
