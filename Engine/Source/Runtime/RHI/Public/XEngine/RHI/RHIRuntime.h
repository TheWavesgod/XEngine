// RHIRuntime — backend factory registry.
//
// Multi-RHI-backend runtime selection. One process, one RHIRuntime. Backends
// (VulkanRHI / D3D12RHI / MetalRHI / …) self-register through their own
// public factory function (see XEngine::VulkanRHI::GetFactory / Register),
// then App picks one at startup and calls RHIRuntime::CreateInstance.
//
// Why this lives in XEngineRHI (not XEngineRHILoader):
//   XEngineRHI holds the *protocol declaration*; XEngineRHILoader (separate
//   target) holds the *implementation*. Splitting keeps XEngineRHI from
//   depending on any backend — see plan §4.
//
// Forward compatibility with DLL loading (plan: Forward Compatibility with DLL):
//   * RHIBackendFactoryFn is a plain function pointer — directly dlsym-able.
//   * XEngineRHIBackendFactoryC is the extern "C" ABI type so a future
//     XEngineRHIBackendFactoryV1 entry point can be looked up from a .dll /
//     .so / .dylib via GetProcAddress / dlsym.
//   * RHIBackendFactoryEntry is pure POD (enum / string_view / u32 / fn ptr)
//     so its binary layout is stable across compiler versions.

#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>

#include <memory>
#include <string_view>
#include <vector>

// ---------------------------------------------------------------------------
// C-compatible factory function pointer type.
//
// Pure POD C signature so that:
//   1. Future dlsym / GetProcAddress lookups against a backend .dll / .so /
//      .dylib can bind directly to the same signature, and
//   2. The owning unique_ptr wrapping in the C++ world is built in
//      RHIRuntime::CreateInstance, not exposed to the C ABI.
//
// Argument is passed as a pointer (rather than by reference) so the ABI
// exactly matches an extern "C" function.
// ---------------------------------------------------------------------------
extern "C"
{
    using XEngineRHIBackendFactoryC = XEngine::RHIInstance*(*)(const XEngine::RHIInstanceDesc*);
}

namespace XEngine
{
    // C++ factory function pointer. Used in-process for now; a future
    // RHIRuntime::LoadDynamicLibrary implementation will convert from
    // XEngineRHIBackendFactoryC (obtained via dlsym) into this type.
    using RHIBackendFactoryFn =
        std::unique_ptr<RHIInstance>(*)(const RHIInstanceDesc&);

    // Backend registration entry. Pure POD — no constructor / destructor /
    // virtual function — so it can be copied across DLL boundaries without
    // ABI-mismatch risk.
    //
    // Fields:
    //   Backend   — RHIBackend enum value. The registry treats this as the
    //               primary key; registering the same enum twice replaces.
    //   Name      — human-readable name; surfaced through EnumerateBackends
    //               and consumed by ParseBackend. Lifetime is the caller's
    //               responsibility (point at string-literal storage).
    //   Priority  — used only when preference == RHIBackend::None (Auto).
    //               Higher = tried first.
    //   Factory   — the function invoked by CreateInstance.
    struct RHIBackendFactoryEntry
    {
        RHIBackend          Backend   = RHIBackend::None;
        std::string_view    Name      = "";
        u32                 Priority  = 100;
        RHIBackendFactoryFn Factory   = nullptr;
    };

    // RHIRuntime — static-method registry. All state lives in the
    // XEngineRHILoader target's translation unit; this class is the
    // protocol-only contract.
    class RHIRuntime
    {
    public:
        // ──────── Backend side ────────
        // Called by the backend's startup code (or App code that links
        // the backend). If an entry with the same Backend already exists,
        // it is replaced. Thread-safe.
        static void RegisterBackend(const RHIBackendFactoryEntry& entry) noexcept;

        // Removes the registration for `backend` if present. No-op if not
        // registered. Mainly intended for tests and DLL unload paths.
        static void UnregisterBackend(RHIBackend backend) noexcept;

        // ──────── App side ────────
        // Picks a registered factory and creates an instance.
        //   preference == RHIBackend::None  : iterate all entries sorted
        //                                     by descending Priority; first
        //                                     Factory that returns non-null
        //                                     wins.
        //   preference != RHIBackend::None  : exact lookup by enum; nullptr
        //                                     if not registered or factory
        //                                     returned nullptr. Never
        //                                     silently falls back to a
        //                                     different backend.
        //
        // Returns nullptr on failure. Caller owns the returned instance.
        static std::unique_ptr<RHIInstance> CreateInstance(
            const RHIInstanceDesc& desc,
            RHIBackend             preference = RHIBackend::None) noexcept;

        // ──────── Query ────────
        // Snapshot of currently-registered factories. Order is unspecified;
        // use Priority to sort.
        static std::vector<RHIBackendFactoryEntry> EnumerateBackends() noexcept;

        // String <-> enum, case-insensitive:
        //   "Vulkan" / "vulkan" / "VK" / "Vk" → RHIBackend::Vulkan
        //   "D3D12"  / "d3d12"  / "DX12" / "dx12" → RHIBackend::D3D12
        //   "Metal"  / "metal"  / "MTL"          → RHIBackend::Metal
        //   "Auto"   / "" / "Default"            → RHIBackend::None
        //   anything else                        → RHIBackend::None
        //                                            (with XENGINE_LOG_WARN)
        static RHIBackend ParseBackend(std::string_view name) noexcept;

        // Returns the canonical lowercase name: "none" / "vulkan" / "d3d12" /
        // "metal". Inverse of ParseBackend for the four enum values.
        static std::string_view GetBackendName(RHIBackend backend) noexcept;
    };
}