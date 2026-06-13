#pragma once

#include <XEngine/Renderer/RenderScene.h>

namespace XEngine
{
    class AssetSystem;
    class Scene;
    struct RenderResourceContext;

    // Converts Scene AssetHandle references into renderer handles.
    // This is the boundary between Scene data and renderer-owned GPU resources.
    class RenderExtraction
    {
    public:
        static void Extract(
            const Scene& scene,
            AssetSystem& assetSystem,
            RenderResourceContext& resources,
            RenderScene& outRenderScene);
    };
}
