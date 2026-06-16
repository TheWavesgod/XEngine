#include "ViewportPanel.h"

#include "../Viewport/ViewportAxisGizmo.h"

#include <imgui.h>

namespace XEngine
{
    void ViewportPanel::Draw(EditorContext& context)
    {
        if (!context.ShowViewport)
        {
            context.ViewportHovered = false;
            context.ViewportFocused = false;
            return;
        }

        ImGui::Begin("Viewport", &context.ShowViewport, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        context.ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        context.ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        const ImVec2 contentMin = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 viewportSize(
            available.x > 1.0f ? available.x : 1.0f,
            available.y > 1.0f ? available.y : 1.0f);
        const ImVec2 contentMax(contentMin.x + viewportSize.x, contentMin.y + viewportSize.y);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(contentMin, contentMax, IM_COL32(28, 30, 34, 255));
        drawList->AddRect(contentMin, contentMax, IM_COL32(80, 86, 96, 255));

        ImGui::InvisibleButton("ViewportRegion", viewportSize);

        const bool canCapture =
            context.UseEditorCamera &&
            ImGui::IsItemHovered() &&
            !ImGui::GetIO().WantTextInput;
        if (canCapture && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            context.ViewportInputMode = ViewportInputMode::CameraCapture;
        }

        const bool captured = context.ViewportInputMode == ViewportInputMode::CameraCapture;
        const char* hint = captured ?
            "Camera Control Mode - Press Esc to release mouse" :
            "Click viewport to control Editor Camera";
        drawList->AddText(
            ImVec2(contentMin.x + 12.0f, contentMin.y + 12.0f),
            IM_COL32(235, 235, 235, 230),
            hint);

        if (!context.UseEditorCamera)
        {
            drawList->AddText(
                ImVec2(contentMin.x + 12.0f, contentMin.y + 34.0f),
                IM_COL32(255, 210, 120, 230),
                "Scene Camera mode");
        }

        DrawViewportAxisGizmo(
            context.Camera,
            ViewportAxisGizmoRect {
                contentMin.x,
                contentMin.y,
                viewportSize.x,
                viewportSize.y });

        ImGui::End();
    }
}
