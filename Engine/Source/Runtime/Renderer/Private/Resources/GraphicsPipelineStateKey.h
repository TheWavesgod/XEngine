#pragma once

#include <XEngine/Renderer/MaterialTypes.h>
#include <XEngine/RHI/RHITypes.h>

#include <cstddef>
#include <functional>

namespace XEngine
{
    enum class RenderPassKind
    {
        ForwardOpaque,
        ShadowDepth,
        PostProcess
    };

    enum class VertexLayoutKind
    {
        MeshVertex
    };

    struct GraphicsPipelineStateKey
    {
        RenderPassKind PassKind = RenderPassKind::ForwardOpaque;
        MaterialShadingModel ShadingModel = MaterialShadingModel::Lit;
        MaterialAlphaMode AlphaMode = MaterialAlphaMode::Opaque;
        VertexLayoutKind VertexLayout = VertexLayoutKind::MeshVertex;
        RHIFormat ColorFormat = RHIFormat::Undefined;
        RHIFormat DepthFormat = RHIFormat::D32Float;
        bool DepthTestEnabled = true;
        bool DepthWriteEnabled = true;
        bool BlendEnabled = false;
        bool DoubleSided = false;
        bool HasColorAttachment = true;
        bool EnableDepthBias = false;
        f32 DepthBiasConstantFactor = 0.0f;
        f32 DepthBiasClamp = 0.0f;
        f32 DepthBiasSlopeFactor = 0.0f;

        // Bump this layout version when shader-visible bind group layout changes.
        u32 PipelineLayoutVersion = 1;

        bool operator==(const GraphicsPipelineStateKey& other) const = default;
    };

    struct GraphicsPipelineStateKeyHash
    {
        std::size_t operator()(const GraphicsPipelineStateKey& key) const
        {
            std::size_t value = 0;
            
            auto FloatBits = [](f32 v) 
            {
                v = (v == 0.0f) ? 0.0f : v;
                return std::bit_cast<int>(v);
            };

            const auto combine = [&value](std::size_t part)
            {
                value ^= part + 0x9e3779b9u + (value << 6u) + (value >> 2u);
            };
            combine(std::hash<int> {}(static_cast<int>(key.PassKind)));
            combine(std::hash<int> {}(static_cast<int>(key.ShadingModel)));
            combine(std::hash<int> {}(static_cast<int>(key.AlphaMode)));
            combine(std::hash<int> {}(static_cast<int>(key.VertexLayout)));
            combine(std::hash<int> {}(static_cast<int>(key.ColorFormat)));
            combine(std::hash<int> {}(static_cast<int>(key.DepthFormat)));
            combine(std::hash<bool> {}(key.DepthTestEnabled));
            combine(std::hash<bool> {}(key.DepthWriteEnabled));
            combine(std::hash<bool> {}(key.BlendEnabled));
            combine(std::hash<bool> {}(key.DoubleSided));
            combine(std::hash<bool> {}(key.HasColorAttachment));
            combine(std::hash<bool> {}(key.EnableDepthBias));
            combine(std::hash<int> {}(key.DepthBiasConstantFactor));
            combine(std::hash<int> {}(key.DepthBiasClamp));
            combine(std::hash<int> {}(key.DepthBiasSlopeFactor));
            combine(std::hash<u32> {}(key.PipelineLayoutVersion));
            return value;
        }
    };
}
