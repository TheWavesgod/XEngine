#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class ShaderResourceType
    {
        Unknown,
        UniformBuffer,
        StorageBuffer,
        Texture,
        Sampler,
        CombinedImageSampler,
        PushConstant
    };

    struct ShaderBindingLocation
    {
        u32 Set = 0;
        u32 Binding = 0;
        u32 Space = 0;
        u32 Register = 0;
        u32 Index = 0;
    };

    struct ShaderResourceBinding
    {
        std::string Name;
        ShaderResourceType Type = ShaderResourceType::Unknown;

        ShaderBindingLocation Location;

        u32 ArraySize = 1;

        ShaderStage Visibility = ShaderStage::Unknown;
    };

    struct ShaderReflection
    {
        std::vector<ShaderResourceBinding> Resources;
    };
}
