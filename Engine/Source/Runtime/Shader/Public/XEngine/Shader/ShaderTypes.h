#pragma once

#include <XEngine/Core/Types.h>

#include <vector>

namespace XEngine
{
    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute
    };

    enum class ShaderTarget
    {
        SpirV,
        Dxil,
        MetalLib
    };

    enum class ShaderResourceType
    {
        UniformBuffer,
        StorageBuffer,
        Texture,
        Sampler
    };

    struct ShaderResourceBinding
    {
        ShaderResourceType Type = ShaderResourceType::UniformBuffer;
        u32 Set = 0;
        u32 Binding = 0;
    };

    struct CompiledShader
    {
        ShaderStage Stage = ShaderStage::Vertex;
        std::vector<u8> Bytecode;
    };
}
