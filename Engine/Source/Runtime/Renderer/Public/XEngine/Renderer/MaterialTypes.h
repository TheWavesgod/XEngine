#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/Texture.h>

namespace XEngine
{
    enum class MaterialShadingModel
    {
        Unlit,
        Lit
    };

    enum class MaterialAlphaMode
    {
        Opaque,
        Masked,
        Blend
    };

    struct MaterialDesc
    {
        MaterialShadingModel ShadingModel = MaterialShadingModel::Lit;
        MaterialAlphaMode AlphaMode = MaterialAlphaMode::Opaque;

        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };

        f32 MetallicFactor = 0.0f;
        f32 RoughnessFactor = 1.0f;
        f32 AlphaCutoff = 0.5f;
        f32 Padding0 = 0.0f;

        TextureHandle BaseColorTexture {};
        TextureHandle NormalTexture {};
        TextureHandle MetallicRoughnessTexture {};
        TextureHandle AOTexture {};

        bool DoubleSided = false;
    };

    struct GPUMaterialData
    {
        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };

        f32 MetallicFactor = 0.0f;
        f32 RoughnessFactor = 1.0f;
        f32 AlphaCutoff = 0.5f;
        f32 Padding0 = 0.0f;

        // Bindless-ready placeholders. Stage 6C does not create bindless descriptors.
        u32 BaseColorTextureIndex = 0;
        u32 NormalTextureIndex = 0;
        u32 MetallicRoughnessTextureIndex = 0;
        u32 AOTextureIndex = 0;

        u32 Flags = 0;
        u32 Padding1 = 0;
        u32 Padding2 = 0;
        u32 Padding3 = 0;
    };
}
