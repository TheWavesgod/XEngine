#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <cstddef>
#include <string>

namespace XEngine
{
    struct RHIShaderDesc
    {
        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::Unknown;
        ShaderCodeFormat Format = ShaderCodeFormat::Unknown;

        std::string EntryPoint;

        const u8* Code = nullptr;
        std::size_t CodeSize = 0;

        const char* DebugName = nullptr;
    };

    class RHIShader
    {
    public:
        virtual ~RHIShader() = default;

        virtual ShaderStage GetStage() const = 0;
        virtual ShaderTarget GetTarget() const = 0;
    };
}
