#pragma once

#include "RenderShaderKey.h"

#include <memory>
#include <unordered_map>

namespace XEngine
{
    class RHIDevice;
    class RHIShader;
    class ShaderSystem;

    class RenderShaderLibrary
    {
    public:
        bool Initialize(RHIDevice* device, ShaderSystem* shaderSystem);
        void Shutdown();

        RHIShader* GetOrCreateShader(const RenderShaderKey& key);

    private:
        std::shared_ptr<RHIShader> CreateShader(const RenderShaderKey& key);

        RHIDevice* m_Device = nullptr;
        ShaderSystem* m_ShaderSystem = nullptr;
        std::unordered_map<RenderShaderKey, std::shared_ptr<RHIShader>, RenderShaderKeyHash> m_Shaders;
    };
}
