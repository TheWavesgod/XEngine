#pragma once

#include <XEngine/Shader/ShaderCompiler.h>

namespace XEngine
{
    class SlangCompiler final : public ShaderCompiler
    {
    public:
        SlangCompiler();
        ~SlangCompiler() override;

        bool IsAvailable() const override;

        CompiledShader Compile(const ShaderCompileDesc& desc) override;

    private:
        bool Initialize();
        void Shutdown();

        bool m_Initialized = false;
    };
}
