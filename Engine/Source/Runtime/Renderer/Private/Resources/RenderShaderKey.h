#pragma once

#include <XEngine/Shader/ShaderTypes.h>

#include <cstddef>
#include <functional>
#include <string>

namespace XEngine
{
    struct RenderShaderKey
    {
        std::string Path;
        std::string EntryPoint;
        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::VulkanSPIRV;
        bool GenerateDebugInfo = false;
        bool EnableOptimization = true;

        bool operator==(const RenderShaderKey& other) const = default;
    };

    struct RenderShaderKeyHash
    {
        std::size_t operator()(const RenderShaderKey& key) const
        {
            std::size_t value = std::hash<std::string> {}(key.Path);
            const auto combine = [&value](std::size_t part)
            {
                value ^= part + 0x9e3779b9u + (value << 6u) + (value >> 2u);
            };
            combine(std::hash<std::string> {}(key.EntryPoint));
            combine(std::hash<int> {}(static_cast<int>(key.Stage)));
            combine(std::hash<int> {}(static_cast<int>(key.Target)));
            combine(std::hash<bool> {}(key.GenerateDebugInfo));
            combine(std::hash<bool> {}(key.EnableOptimization));
            return value;
        }
    };
}
