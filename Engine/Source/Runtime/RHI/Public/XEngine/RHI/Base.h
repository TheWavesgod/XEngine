#pragma once

#include <XEngine/Core/Types.h>

// Reuse the project's generic Result type. The RHI layer adopts the same
// success/failure pattern that the rest of the engine uses; the RHI layer
// does not introduce any Vulkan, volk, VMA, SDL3, Shader, or Renderer
// dependency through this header.
#include <XEngine/Core/Result.h>

#include <XEngine/RHI/Export.h>

#include <cstdint>

// The new RHI is a from-scratch rebuild. This header establishes the
// minimal public surface: an export macro, a version sentinel, a backend
// tag, and an alias for the project's generic Result type. Instance,
// Adapter, Device, Queue, Resource, CommandList, Pipeline and Swapchain
// are intentionally NOT declared here yet — they will be added by later
// protocol-design stages per Docs/AI helper/prompts.md.
namespace XEngine
{
    inline constexpr u32 RHIVersionMajor = 0;
    inline constexpr u32 RHIVersionMinor = 3;  // M3: RHIDevice + Queue + Capabilities + feature negotiation.
    inline constexpr u32 RHIVersionPatch = 0;

    enum class RHIBackend : std::uint8_t
    {
        None = 0,
        Vulkan,
        // D3D12 / Metal are intentionally not yet enumerated here; they
        // will only land once a real backend target exists. See
        // Docs/AI helper/prompts.md for the strict "no empty stubs" rule.
    };

    using RHIResult = XEngine::Result;
} // namespace XEngine

// Version-probe and identifier-entry symbol exports. Declared here with
// explicit C linkage so callers in C++ TUs can call them by their plain C
// names without name mangling. Implementations live in Private/RHI.cpp.
extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionMajor();
extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionMinor();
extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionPatch();
