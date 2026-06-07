#pragma once

#include <XEngine/Asset/AssetHandle.h>

namespace XEngine
{
    // Scene renderable component that references CPU-side assets.
    // Renderer MeshHandle and MaterialHandle are produced later by RenderExtraction.
    struct MeshRendererComponent
    {
        AssetHandle MeshAsset;
        AssetHandle MaterialAsset;

        bool Visible = true;
        bool CastShadow = true;
        bool ReceiveShadow = true;
    };
}
