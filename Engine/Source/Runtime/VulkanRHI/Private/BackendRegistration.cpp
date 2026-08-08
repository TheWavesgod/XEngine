// BackendRegistration — Vulkan backend's plug-in surface for XEngine::RHIRuntime.
//
// Implements XEngine::VulkanRHI::GetFactory and XEngine::VulkanRHI::Register.
// The actual VulkanInstance::CreateInstance work lives in VulkanInstance.cpp;
// this file is a thin bridge that lets the backend self-describe to the
// central registry without polluting the public header with RHIRuntime types
// other than the minimal protocol (RHIBackendFactoryFn, RHIBackendFactoryEntry).

#include <XEngine/VulkanRHI/Backend.h>

namespace XEngine::VulkanRHI
{
    RHIBackendFactoryFn GetFactory() noexcept
    {
        // Re-use the existing free function. Returning the function pointer
        // directly (rather than wrapping) keeps the indirection to zero and
        // lets future dlsym logic retrieve the same pointer under a stable
        // C ABI name.
        return &CreateInstance;
    }

    void Register() noexcept
    {
        RHIRuntime::RegisterBackend({
            .Backend  = RHIBackend::Vulkan,
            .Name     = "Vulkan",
            .Priority = 100,
            .Factory  = &CreateInstance,
        });
    }
}