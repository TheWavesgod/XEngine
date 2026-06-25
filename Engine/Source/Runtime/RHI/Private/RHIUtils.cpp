#include "XEngine/RHI/RHIUtils.h"

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend)
    {
        switch (backend)
        {
        case RHIBackend::None:   return "None";
        case RHIBackend::Vulkan: return "Vulkan";
        case RHIBackend::D3D12:  return "D3D12";
        case RHIBackend::Metal:  return "Metal";
        }
        return "Unknown";
    }
}