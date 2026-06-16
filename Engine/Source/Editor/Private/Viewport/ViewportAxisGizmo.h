#pragma once

#include <XEngine/Editor/EditorCamera.h>

namespace XEngine
{
    struct ViewportAxisGizmoRect
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
    };

    void DrawViewportAxisGizmo(const EditorCamera& camera, const ViewportAxisGizmoRect& rect);
}
