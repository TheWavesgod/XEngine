// BackendRegistration — D3D12 backend's plug-in surface for XEngine::RHIRuntime.
//
// Implements XEngine::D3D12RHI::GetFactory and XEngine::D3D12RHI::Register.
// The actual D3D12Instance::Create work lives in D3D12Instance.cpp; this
// file is a thin bridge that lets the backend self-describe to the central
// registry without polluting the public header with RHIRuntime types other
// than the minimal protocol (RHIBackendFactoryFn, RHIBackendFactoryEntry).

#include <XEngine/D3D12RHI/Backend.h>

namespace XEngine::D3D12RHI
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
            .Backend  = RHIBackend::D3D12,
            .Name     = "D3D12",
            .Priority = 100,
            .Factory  = &CreateInstance,
        });
    }
}