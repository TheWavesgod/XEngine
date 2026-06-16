#pragma once

#include <XEngine/Editor/EditorContext.h>

namespace XEngine
{
    class SceneHierarchyPanel
    {
    public:
        void Draw(EditorContext& context);

    private:
        void DrawEntityNode(EditorContext& context, Entity entity);
    };
}
