#pragma once

#include <XEngine/Renderer/RenderScene.h>

namespace XEngine
{
    class AssetSystem;
    class MaterialSystem;
    class RenderMeshManager;
    class Scene;
    class TextureManager;

    // Converts Scene AssetHandle references into renderer handles.
    // This is the boundary between Scene data and renderer-owned GPU resources.
    class RenderExtraction
    {
    public:
        static void Extract(
            const Scene& scene,
            AssetSystem& assetSystem,
            RenderMeshManager& meshManager,
            MaterialSystem& materialSystem,
            TextureManager& textureManager,
            RenderScene& outRenderScene);
    };
}
