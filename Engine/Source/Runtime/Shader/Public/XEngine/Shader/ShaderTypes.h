#pragma once

#include <XEngine/Core/Types.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class ShaderStage
    {
        Unknown,
        Vertex,
        Fragment,
        Compute
    };

    enum class ShaderTarget
    {
        Unknown,
        VulkanSPIRV,
        D3D12DXIL,
        MetalMSL
    };

    enum class ShaderCodeFormat
    {
        Unknown,
        Binary,
        Text
    };

    enum class ShaderCompileResult
    {
        Success,
        Failed,
        UnsupportedTarget,
        CompilerUnavailable
    };

    struct ShaderDefine
    {
        std::string Name;
        std::string Value;
    };

    struct ShaderCompileDesc
    {
        std::string Path;
        std::string EntryPoint;

        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::VulkanSPIRV;

        std::string Profile;

        std::vector<std::string> IncludeDirectories;
        std::vector<ShaderDefine> Defines;

        bool GenerateDebugInfo = true;
        bool EnableOptimization = false;
    };

    ShaderTarget ShaderTargetFromRHIBackendName(const std::string& backendName);
}
