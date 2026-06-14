#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/Texture.h>

namespace XEngine
{
    // Renderer-side shading category used by RenderMaterialSystem and pipelines.
    enum class MaterialShadingModel
    {
        Unlit,
        Lit
    };

    // Renderer-side alpha behavior. Full transparent sorting is handled by later stages.
    enum class MaterialAlphaMode
    {
        Opaque,
        Masked,
        Blend
    };

    // Renderer-facing material description. It owns TextureHandle references only;
    // Asset MaterialAsset data is converted into this by RenderMaterialSystem.
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

    // GPU upload layout for material constants and bindless-ready texture indices.
    // Stage 7D still uses classic bind groups for actual texture binding.
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
