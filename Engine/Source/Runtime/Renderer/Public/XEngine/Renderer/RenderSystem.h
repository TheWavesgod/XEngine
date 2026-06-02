#pragma once

#include <XEngine/Engine/Subsystem.h>

namespace XEngine
{
    struct RenderScene;

    class RenderSystem : public ISubsystem
    {
    public:
        void BeginFrame();
        void Submit(const RenderScene& scene);
        void Render();
        void EndFrame();
    };
}
