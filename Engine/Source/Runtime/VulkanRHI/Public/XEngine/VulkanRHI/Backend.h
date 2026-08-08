// VulkanRHI public backend factory API.
//
// Phase 1 (M0-M3 backend): exposes VulkanInstance creation so consumers
// (apps, integration tests) can get a Vulkan-backed RHIInstance without
// reaching into the Private/ directory. The factory returns nullptr if
// the Vulkan SDK is unavailable or instance creation fails.
//
// Phase 2 (M2 multi-backend): also exposes factory-function and Register()
// helpers so App code can plug Vulkan into XEngine::RHIRuntime
// (see <XEngine/RHI/RHIRuntime.h>).
//
// Future phases (M4+, M11+) will add factories for other backend types
// (physical-device enumeration via the instance, etc.).

#pragma once

#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIRuntime.h>

#include <memory>

namespace XEngine
{
    // Concrete RHIInstance implementation for the Vulkan backend.
    class VulkanInstance;

    namespace VulkanRHI
    {
        // Creates a VkInstance via volk, wraps it in a VulkanInstance
        // (which is an RHIInstance subclass), and returns it.
        // Returns nullptr if Vulkan SDK is unavailable or instance creation fails.
        std::unique_ptr<RHIInstance> CreateInstance(const RHIInstanceDesc& desc);

        // Returns the C++ factory function pointer that RHIRuntime calls
        // when dispatching CreateInstance. Equivalent to &CreateInstance
        // — exposed as a named function for symmetry with future backends
        // (D3D12RHI::GetFactory, MetalRHI::GetFactory) and for future
        // DLL loading where dlsym will retrieve a similar function
        // pointer (see plan: Forward Compatibility with DLL).
        XEngine::RHIBackendFactoryFn GetFactory() noexcept;

        // Registers the Vulkan backend with XEngine::RHIRuntime under the
        // RHIBackend::Vulkan key. App code calls this once during startup,
        // BEFORE any RHIRuntime::CreateInstance call. Idempotent — calling
        // twice replaces the registration, which is also the semantic used
        // by future DLL loaders to override an existing entry.
        //
        // Typical App startup pattern:
        //     XEngine::VulkanRHI::Register();
        //     auto inst = XEngine::RHIRuntime::CreateInstance(desc, preference);
        void Register() noexcept;
    }
}
