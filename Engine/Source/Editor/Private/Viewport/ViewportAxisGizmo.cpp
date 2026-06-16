#include "ViewportAxisGizmo.h"

#include <XEngine/Math/CoordinateSystem.h>
#include <XEngine/Math/MathFunctions.h>

#include <imgui.h>

namespace XEngine
{
    namespace
    {
        ImVec2 ProjectAxis(const Quat& inverseCameraRotation, const Vec3& axis, float length)
        {
            const Vec3 local = inverseCameraRotation * axis;
            return ImVec2(local.y * length, -local.z * length);
        }
    }

    void DrawViewportAxisGizmo(const EditorCamera& camera, const ViewportAxisGizmoRect& rect)
    {
        // This is a screen-space editor overlay for orientation feedback, not
        // the runtime DebugDraw system.
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr || rect.Width <= 0.0f || rect.Height <= 0.0f)
        {
            return;
        }

        const ImVec2 origin(rect.X + 56.0f, rect.Y + rect.Height - 56.0f);
        const float length = 32.0f;
        const Quat inverseRotation = Math::Inverse(camera.GetRotation());

        struct AxisStyle
        {
            Vec3 Axis;
            ImU32 Color;
            const char* Label;
        };

        const AxisStyle axes[] = {
            { CoordinateSystem::Forward, IM_COL32(230, 80, 70, 255), "X" },
            { CoordinateSystem::Right, IM_COL32(80, 210, 90, 255), "Y" },
            { CoordinateSystem::Up, IM_COL32(90, 140, 255, 255), "Z" }
        };

        for (const AxisStyle& axis : axes)
        {
            const ImVec2 offset = ProjectAxis(inverseRotation, axis.Axis, length);
            const ImVec2 end(origin.x + offset.x, origin.y + offset.y);
            drawList->AddLine(origin, end, axis.Color, 2.0f);
            drawList->AddCircleFilled(end, 3.0f, axis.Color);
            drawList->AddText(ImVec2(end.x + 4.0f, end.y - 7.0f), axis.Color, axis.Label);
        }

        drawList->AddText(
            ImVec2(origin.x - 34.0f, origin.y + 22.0f),
            IM_COL32(220, 220, 220, 210),
            "+X Forward  +Y Right  +Z Up");
    }
}
