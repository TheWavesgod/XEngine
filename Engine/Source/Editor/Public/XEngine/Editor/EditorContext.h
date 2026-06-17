#pragma once

#include <XEngine/Editor/EditorCamera.h>
#include <XEngine/Renderer/RendererDebugSettings.h>
#include <XEngine/Scene/Entity.h>
#include <XEngine/Core/Types.h>

#include <filesystem>

namespace XEngine
{
    class AssetSystem;
    class Scene;

    enum class ViewportInputMode
    {
        UI,
        CameraCapture
    };

    enum class TransformEditSpace
    {
        World,
        Local
    };

    struct EditorContext
    {
        Scene* ActiveScene = nullptr;
        AssetSystem* Assets = nullptr;
        RendererDebugSettings* RendererDebug = nullptr;
        Entity SelectedEntity {};

        // Selection, dirty state, and editor camera mode are editor state, not
        // serialized runtime Scene data.
        std::filesystem::path CurrentScenePath;
        bool SceneDirty = false;
        // ViewportInputMode separates ordinary ImGui interaction from explicit
        // camera capture, so text fields/sliders do not move the editor camera.
        ViewportInputMode ViewportInputMode = ViewportInputMode::UI;
        bool ViewportHovered = false;
        bool ViewportFocused = false;
        u32 ViewportWidth = 1;
        u32 ViewportHeight = 1;
        u64 ViewportTextureId = 0;
        bool UseEditorCamera = true;
        EditorCamera Camera;
        TransformEditSpace TransformSpace = TransformEditSpace::World;

        bool ShowSceneHierarchy = true;
        bool ShowInspector = true;
        bool ShowRendererDebug = true;
        bool ShowViewport = true;
        bool ResetDockingLayoutRequested = false;
    };
}
