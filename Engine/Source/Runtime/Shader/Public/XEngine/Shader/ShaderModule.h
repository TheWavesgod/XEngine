#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderReflection.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <string>
#include <vector>

namespace XEngine
{
    struct CompiledShader
    {
        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::Unknown;
        ShaderCodeFormat Format = ShaderCodeFormat::Unknown;

        std::string EntryPoint;
        std::string SourcePath;

        std::vector<u8> Bytecode;
        std::string SourceCode;

        ShaderReflection Reflection;

        ShaderCompileResult Result = ShaderCompileResult::Failed;
        std::string Diagnostics;

        bool IsValid() const
        {
            return Result == ShaderCompileResult::Success && (!Bytecode.empty() || !SourceCode.empty());
        }
    };
}
