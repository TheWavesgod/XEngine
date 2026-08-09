# -----------------------------------------------------------------------------
# Engine/CMakeLists.txt — post-RHI-rebuild configuration.
#
# Per Docs/AI helper/prompts.md the rebuilt engine now has clearly separable
# modules:
#
#   Foundation     : always built.
#   RHI            : XEngineRHI              (XENGINE_BUILD_RHI)
#   VulkanRHI      : XEngineVulkanRHI        (XENGINE_BUILD_VULKAN_RHI)
#   Tests          : Unit / Integration / Smoke (XENGINE_BUILD_TESTS)
#
# Everything else (Engine runtime, Asset, Scene, Input, Serialization,
# Shader, Platform, Renderer, Editor) is intentionally NOT pulled in
# while the new RHI is being established. Those modules will be
# re-anchored to XEngineRHI in later refactor stages.
# -----------------------------------------------------------------------------

# Foundation: always required. The new RHI / VulkanRHI / Tests all link
# against XEngineFoundation directly.
add_subdirectory(Source/Foundation)

# New XEngineRHI target. Independent from everything else; no Renderer,
# no Engine runtime, no Asset / Scene / Shader.
if(XENGINE_BUILD_RHI)
    add_subdirectory(Source/Runtime/RHI)
endif()

# New XEngineRHILoader target — implements XEngine::RHIRuntime
# (backend factory registry). Depends PUBLIC on XEngineRHI; PRIVATE no
# backend headers. Sits between RHI and the backends, MUST be added
# BEFORE any backend so that backend CMakeLists can PUBLIC-link it.
if(XENGINE_BUILD_RHI_LOADER)
    add_subdirectory(Source/Runtime/RHILoader)
endif()

# D3D12RHI gating note: XENGINE_BUILD_D3D12_RHI itself is honored only on
# WIN32. Off-Windows builds skip the target entirely. We still need to
# explicitly no-op the gate here so other CMakeLists additions remain
# aware that D3D12RHI exists as an optional path.
if(XENGINE_BUILD_D3D12_RHI AND NOT WIN32)
    message(STATUS
        "XENGINE_BUILD_D3D12_RHI=ON but platform is not WIN32 — "
        "skipping XEngineD3D12RHI target (D3D12 is Windows-only).")
endif()

# New XEngineVulkanRHI target. PUBLIC depends on XEngineRHI + XEngineRHILoader;
# PRIVATE depends on Vulkan SDK + volk + VMA (wired inside its own CMakeLists).
if(XENGINE_BUILD_VULKAN_RHI)
    add_subdirectory(Source/Runtime/VulkanRHI)
endif()

# New XEngineD3D12RHI target. PUBLIC depends on XEngineRHI + XEngineRHILoader;
# PRIVATE depends on d3d12.lib / dxgi.lib from the Windows SDK (wired inside
# its own CMakeLists). The target is gated by both XENGINE_BUILD_D3D12_RHI
# and the WIN32 platform — there is no D3D12 backend on other OSes.
if(XENGINE_BUILD_D3D12_RHI AND WIN32)
    add_subdirectory(Source/Runtime/D3D12RHI)
endif()

# Tests: gated by XENGINE_BUILD_TESTS. The Tests/CMakeLists tree already
# has its own BUILD_UNIT_TESTS / BUILD_INTEGRATION_TESTS /
# BUILD_SMOKE_TESTS / BUILD_VULKAN_TESTS gating inside.
if(XENGINE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CMakeLists.txt")
    add_subdirectory(Tests)
endif()

# -----------------------------------------------------------------------------
# The blocks below are intentionally disabled by default. They are kept here
# as documentation of the original layout so that future re-enablement is a
# single-flip operation. Each block is gated by its module-level option and
# also checks that the corresponding source tree still exists.
# -----------------------------------------------------------------------------

if(XENGINE_BUILD_RENDERER)
    # Originally:
    #   add_subdirectory(Source/Runtime/Engine)
    #   add_subdirectory(Source/Runtime/Serialization)
    #   add_subdirectory(Source/Runtime/Asset)
    #   add_subdirectory(Source/Runtime/Input)
    #   add_subdirectory(Source/Runtime/Scene)
    #   add_subdirectory(Source/Runtime/Platform)
    #   add_subdirectory(Source/Runtime/Shader)
    #   add_subdirectory(Source/Runtime/Renderer)
    #
    # Each of those modules depended on the old monolithic XEngineRHI and is
    # therefore frozen until the new RHI stabilises.
    message(WARNING
        "XENGINE_BUILD_RENDERER=ON is not yet wired to the new RHI. "
        "Enable only after the Renderer has been re-anchored on XEngineRHI.")
endif()

if(XENGINE_BUILD_EDITOR)
    # Originally: an inline `add_library(XEngineEditor ${XENGINE_EDITOR_SOURCES})`
    # plus a link to XEngineRuntime + ThirdParty_imgui. Frozen because the
    # Editor consumed Vulkan Native bridge types from the old RHI.
    message(WARNING
        "XENGINE_BUILD_EDITOR=ON is not yet wired to the new RHI. "
        "Enable only after the Editor ImGui bridge has been redesigned.")
endif()
