#pragma once

#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderTypes.h>

namespace XEngine
{
    class ShaderCompiler
    {
    public:
        virtual ~ShaderCompiler() = default;

        virtual bool IsAvailable() const = 0;

        virtual CompiledShader Compile(const ShaderCompileDesc& desc) = 0;
    };
}
