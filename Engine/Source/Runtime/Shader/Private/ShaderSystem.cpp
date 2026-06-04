#include <XEngine/Shader/ShaderSystem.h>

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Shader/ShaderCompiler.h>

#if defined(XENGINE_ENABLE_SHADER_COMPILER)
    #include "Slang/SlangCompiler.h"
#endif

#include <string>

namespace XEngine
{
    ShaderSystem::ShaderSystem() = default;

    ShaderSystem::~ShaderSystem()
    {
        OnDestroy();
    }

    void ShaderSystem::OnCreate(const SubsystemContext& context)
    {
        XENGINE_LOG_INFO("Creating ShaderSystem");

#if defined(XENGINE_ENABLE_SHADER_COMPILER)
        m_Compiler = std::make_unique<SlangCompiler>();
        if (!m_Compiler->IsAvailable())
        {
            XENGINE_LOG_ERROR("Shader compiler is unavailable");
            return;
        }

        ShaderCompileDesc vertexDesc;
        vertexDesc.Path = "Engine/Shaders/Passes/Triangle.slang";
        vertexDesc.EntryPoint = "vertexMain";
        vertexDesc.Stage = ShaderStage::Vertex;
        vertexDesc.Target = ShaderTarget::VulkanSPIRV;
        vertexDesc.GenerateDebugInfo = true;
        vertexDesc.EnableOptimization = false;

        CompiledShader vertexShader = m_Compiler->Compile(vertexDesc);
        if (!vertexShader.IsValid())
        {
            XENGINE_LOG_ERROR(vertexShader.Diagnostics.empty() ? "Triangle vertex shader compilation failed" :
                                                                  vertexShader.Diagnostics);
            XENGINE_ASSERT(false, "Triangle vertex shader compilation failed");
            return;
        }

        std::string vertexMessage = "Compiled Triangle vertex shader: ";
        vertexMessage += std::to_string(vertexShader.Bytecode.size());
        vertexMessage += " bytes";
        XENGINE_LOG_INFO(vertexMessage);

        ShaderCompileDesc fragmentDesc;
        fragmentDesc.Path = "Engine/Shaders/Passes/Triangle.slang";
        fragmentDesc.EntryPoint = "fragmentMain";
        fragmentDesc.Stage = ShaderStage::Fragment;
        fragmentDesc.Target = ShaderTarget::VulkanSPIRV;
        fragmentDesc.GenerateDebugInfo = true;
        fragmentDesc.EnableOptimization = false;

        CompiledShader fragmentShader = m_Compiler->Compile(fragmentDesc);
        if (!fragmentShader.IsValid())
        {
            XENGINE_LOG_ERROR(fragmentShader.Diagnostics.empty() ? "Triangle fragment shader compilation failed" :
                                                                    fragmentShader.Diagnostics);
            XENGINE_ASSERT(false, "Triangle fragment shader compilation failed");
            return;
        }

        std::string fragmentMessage = "Compiled Triangle fragment shader: ";
        fragmentMessage += std::to_string(fragmentShader.Bytecode.size());
        fragmentMessage += " bytes";
        XENGINE_LOG_INFO(fragmentMessage);
#else
        XENGINE_LOG_WARN("Runtime shader compiler is disabled");
#endif

        m_Initialized = true;
    }

    void ShaderSystem::OnDestroy()
    {
        if (!m_Initialized && !m_Compiler)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying ShaderSystem");
        m_Compiler.reset();
        m_Initialized = false;
    }

    bool ShaderSystem::IsCompilerAvailable() const
    {
        return m_Compiler != nullptr && m_Compiler->IsAvailable();
    }

    CompiledShader ShaderSystem::Compile(const ShaderCompileDesc& desc)
    {
        if (!m_Compiler)
        {
            CompiledShader shader;
            shader.Stage = desc.Stage;
            shader.Target = desc.Target;
            shader.EntryPoint = desc.EntryPoint;
            shader.SourcePath = desc.Path;
            shader.Result = ShaderCompileResult::CompilerUnavailable;
            shader.Diagnostics = "Shader compiler is unavailable";
            return shader;
        }

        return m_Compiler->Compile(desc);
    }
}
