#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <memory>

namespace XEngine
{
    class ShaderCompiler;

    class ShaderSystem final : public ISubsystem
    {
    public:
        ShaderSystem();
        ~ShaderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;

        bool IsCompilerAvailable() const;

        CompiledShader Compile(const ShaderCompileDesc& desc);

    private:
        std::unique_ptr<ShaderCompiler> m_Compiler;
        bool m_Initialized = false;
    };
}
