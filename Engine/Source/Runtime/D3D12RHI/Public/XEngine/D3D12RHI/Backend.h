// D3D12RHI public backend factory API.
//
// Phase 1 (M0-M3 backend, skeleton): exposes D3D12Instance creation so
// consumers (apps, integration tests) can get a D3D12-backed RHIInstance
// without reaching into the Private/ directory. The factory returns
// nullptr if DXGI factory creation fails or the D3D12 device creation
// later surfaces an error.
//
// Phase 2 (M2 multi-backend): also exposes factory-function and Register()
// helpers so App code can plug D3D12 into XEngine::RHIRuntime (see
// <XEngine/RHI/RHIRuntime.h>). The contract is symmetric with
// XEngine::VulkanRHI::CreateInstance / GetFactory / Register so an App
// can mix and match backend registrations.
//
// Future phases will add factories for swapchain / command list / resource
// creation as the protocol surface (M5/M6/M10) lands.

#pragma once

#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIRuntime.h>

#include <memory>

namespace XEngine
{
    // Concrete RHIInstance implementation for the D3D12 backend.
    class D3D12Instance;

    namespace D3D12RHI
    {
        // Creates an IDXGIFactory4 + ID3D12Device, wraps them in a
        // D3D12Instance (which is an RHIInstance subclass), and returns it.
        // Returns nullptr if DXGI factory creation fails or no D3D12
        // device can be created.
        std::unique_ptr<RHIInstance> CreateInstance(const RHIInstanceDesc& desc);

        // Returns the C++ factory function pointer that RHIRuntime calls
        // when dispatching CreateInstance. Equivalent to &CreateInstance
        // — exposed as a named function for symmetry with VulkanRHI
        // and for future DLL loading where dlsym will retrieve a similar
        // function pointer.
        XEngine::RHIBackendFactoryFn GetFactory() noexcept;

        // Registers the D3D12 backend with XEngine::RHIRuntime under the
        // RHIBackend::D3D12 key. App code calls this once during startup,
        // BEFORE any RHIRuntime::CreateInstance call. Idempotent — calling
        // twice replaces the registration, which is also the semantic used
        // by future DLL loaders to override an existing entry.
        //
        // Typical App startup pattern:
        //     XEngine::VulkanRHI::Register();   // (or any other backends)
        //     XEngine::D3D12RHI::Register();
        //     auto inst = XEngine::RHIRuntime::CreateInstance(desc, preference);
        void Register() noexcept;
    }
}