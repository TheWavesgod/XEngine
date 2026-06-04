#include <XEngine/Shader/ShaderTypes.h>

namespace XEngine
{
    ShaderTarget ShaderTargetFromRHIBackendName(const std::string& backendName)
    {
        if (backendName == "Vulkan")
        {
            return ShaderTarget::VulkanSPIRV;
        }

        if (backendName == "D3D12")
        {
            return ShaderTarget::D3D12DXIL;
        }

        if (backendName == "Metal")
        {
            return ShaderTarget::MetalMSL;
        }

        return ShaderTarget::Unknown;
    }
}
