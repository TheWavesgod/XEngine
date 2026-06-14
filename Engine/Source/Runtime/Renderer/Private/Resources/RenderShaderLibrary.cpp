#include "RenderShaderLibrary.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderSystem.h>

#include <string>

namespace XEngine
{
    bool RenderShaderLibrary::Initialize(RHIDevice* device, ShaderSystem* shaderSystem)
    {
        if (device == nullptr || !device->IsValid() || shaderSystem == nullptr ||
            !shaderSystem->IsCompilerAvailable())
        {
            XENGINE_LOG_ERROR("RenderShaderLibrary requires a valid RHIDevice and ShaderSystem");
            return false;
        }

        m_Device = device;
        m_ShaderSystem = shaderSystem;
        XENGINE_LOG_INFO("RenderShaderLibrary initialized");
        return true;
    }

    void RenderShaderLibrary::Shutdown()
    {
        if (!m_Shaders.empty() || m_Device != nullptr)
        {
            XENGINE_LOG_INFO("RenderShaderLibrary shutdown");
        }
        m_Shaders.clear();
        m_ShaderSystem = nullptr;
        m_Device = nullptr;
    }

    RHIShader* RenderShaderLibrary::GetOrCreateShader(const RenderShaderKey& key)
    {
        const auto cached = m_Shaders.find(key);
        if (cached != m_Shaders.end())
        {
            return cached->second.get();
        }

        std::shared_ptr<RHIShader> shader = CreateShader(key);
        if (!shader)
        {
            return nullptr;
        }

        RHIShader* result = shader.get();
        m_Shaders.emplace(key, std::move(shader));
        return result;
    }

    std::shared_ptr<RHIShader> RenderShaderLibrary::CreateShader(const RenderShaderKey& key)
    {
        if (m_Device == nullptr || m_ShaderSystem == nullptr)
        {
            return {};
        }

        ShaderCompileDesc compileDesc;
        compileDesc.Path = key.Path;
        compileDesc.EntryPoint = key.EntryPoint;
        compileDesc.Stage = key.Stage;
        compileDesc.Target = key.Target;
        compileDesc.GenerateDebugInfo = key.GenerateDebugInfo;
        compileDesc.EnableOptimization = key.EnableOptimization;

        CompiledShader compiled = m_ShaderSystem->Compile(compileDesc);
        if (!compiled.IsValid())
        {
            XENGINE_LOG_ERROR(
                compiled.Diagnostics.empty() ?
                    std::string("Shader compilation failed: ") + key.Path + " " + key.EntryPoint :
                    compiled.Diagnostics);
            return {};
        }

        const std::string debugName = key.Path + ":" + key.EntryPoint;
        RHIShaderDesc shaderDesc;
        shaderDesc.Stage = compiled.Stage;
        shaderDesc.Target = compiled.Target;
        shaderDesc.Format = compiled.Format;
        shaderDesc.EntryPoint = "main";
        shaderDesc.Code = compiled.Bytecode.data();
        shaderDesc.CodeSize = compiled.Bytecode.size();
        shaderDesc.DebugName = debugName.c_str();

        std::shared_ptr<RHIShader> shader = m_Device->CreateShader(shaderDesc);
        if (!shader)
        {
            XENGINE_LOG_ERROR(std::string("Failed to create RHI shader: ") + debugName);
            return {};
        }

        XENGINE_LOG_INFO(std::string("RenderShaderLibrary cached shader: ") + debugName);
        return shader;
    }
}
