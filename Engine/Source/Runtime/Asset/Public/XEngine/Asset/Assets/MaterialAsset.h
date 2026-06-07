#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>

#include <string>

namespace XEngine
{
    // CPU-side source material shading category. Renderer pipelines translate this
    // into their own material representation.
    enum class MaterialAssetShadingModel
    {
        Unlit,
        Lit
    };

    // CPU-side alpha behavior imported or authored as asset data.
    enum class MaterialAssetAlphaMode
    {
        Opaque,
        Masked,
        Blend
    };

    // CPU-side material description owned by AssetSystem.
    // It references TextureAsset data by AssetHandle and intentionally contains
    // no renderer handles, bind groups, backend objects, or GPU resources.
    struct MaterialAsset
    {
        std::string Name;
        std::string SourcePath;

        MaterialAssetShadingModel ShadingModel = MaterialAssetShadingModel::Lit;
        MaterialAssetAlphaMode AlphaMode = MaterialAssetAlphaMode::Opaque;

        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };

        f32 MetallicFactor = 0.0f;
        f32 RoughnessFactor = 1.0f;
        f32 AlphaCutoff = 0.5f;
        f32 Padding0 = 0.0f;

        AssetHandle BaseColorTexture {};
        AssetHandle NormalTexture {};
        AssetHandle MetallicRoughnessTexture {};
        AssetHandle AOTexture {};

        bool DoubleSided = false;

        bool IsValid() const
        {
            return !Name.empty();
        }
    };
}
