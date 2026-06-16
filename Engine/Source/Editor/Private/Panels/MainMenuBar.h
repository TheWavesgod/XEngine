#pragma once

#include <XEngine/Editor/EditorContext.h>

namespace XEngine
{
    class MainMenuBar
    {
    public:
        void Draw(EditorContext& context);

    private:
        void NewScene(EditorContext& context);
        void OpenScene(EditorContext& context, const char* path);
        void SaveScene(EditorContext& context, const char* fallbackPath);
        void SaveSceneAs(EditorContext& context, const char* path);
    };
}
