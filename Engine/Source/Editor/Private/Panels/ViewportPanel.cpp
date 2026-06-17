#include "ViewportPanel.h"

#include "../Viewport/ViewportAxisGizmo.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

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

        ImGui::Begin(
            "Viewport",
            &context.ShowViewport,
            ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);

        context.ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        context.ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        const ImVec2 contentMin = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 viewportSize(
            available.x > 1.0f ? available.x : 1.0f,
            available.y > 1.0f ? available.y : 1.0f);
        const ImVec2 contentMax(contentMin.x + viewportSize.x, contentMin.y + viewportSize.y);
        // The panel only requests a resize when its content size changes; the
        // render target is recreated by EditorSystem on the following frame.
        context.ViewportWidth = static_cast<u32>(std::max(1.0f, std::round(viewportSize.x)));
        context.ViewportHeight = static_cast<u32>(std::max(1.0f, std::round(viewportSize.y)));

        if (context.ViewportTextureId != 0)
        {
            ImGui::Image(static_cast<ImTextureID>(context.ViewportTextureId), viewportSize);
        }
        else
        {
            ImGui::InvisibleButton("ViewportImageFallback", viewportSize);
        }

        ImGui::SetCursorScreenPos(contentMin);
        ImGui::InvisibleButton("ViewportRegion", viewportSize);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(contentMin, contentMax, IM_COL32(80, 86, 96, 255));

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
