# New XEngineD3D12RHI target — minimal skeleton only.
#
# Per Docs/AI helper/prompts.md this target establishes a separate build
# identity for the D3D12 backend. It is intentionally minimal at this
# stage; the goal is to verify that the new XEngineD3D12RHI target:
#
#   * Compiles standalone on WIN32.
#   * Links d3d12.lib / dxgi.lib privately (i.e. none of them leak into
#     consumers).
#   * Exposes a clean public header surface (XEngine/D3D12RHI/Base.h,
#     Backend.h, Export.h) that contains zero D3D12 / DXGI / WRL symbols.
#   * Depends on XEngineRHI publicly so its (future) backend factory
#     can live behind a unified RHI namespace.
#   * Self-registers with XEngine::RHIRuntime through D3D12RHI::Register().
#
# Hard rules enforced by review:
#   - D3D12 / DXGI / WRL headers and library links MUST be PRIVATE here.
#   - No Renderer / Editor / Asset / Scene / Shader link.
#   - SDL3 may only be linked from the Smoke test target, never from
#     XEngineD3D12RHI itself.
#   - D3D12-specific compile definitions (D3D12_EXPORT_DECLARATIONS,
#     D3D_DEBUG_NAME_INTERFACE, etc.) are PRIVATE here, not PUBLIC.
#   - This target is gated to WIN32; on other platforms the parent
#     CMakeLists (Engine/CMakeLists.txt) skips this subdirectory entirely.

file(GLOB_RECURSE XENGINE_D3D12_RHI_SOURCES
    CONFIGURE_DEPENDS
        Public/*.h
        Private/*.cpp
)

add_library(XEngineD3D12RHI ${XENGINE_D3D12_RHI_SOURCES})

target_include_directories(XEngineD3D12RHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(XEngineD3D12RHI PUBLIC cxx_std_20)

# The new XEngineD3D12RHI is currently a static library, so its export
# macro is a no-op. Define XENGINE_D3D12_RHI_STATIC when building this
# target so the XENGINE_D3D12_RHI_API tokens in Export.h resolve to empty.
target_compile_definitions(XEngineD3D12RHI
    PUBLIC
        XENGINE_D3D12_RHI_STATIC
)

target_link_libraries(XEngineD3D12RHI
    PUBLIC
        XEngineRHI
        XEngineRHILoader
    PRIVATE
        XEngineFoundation
)

# Expose a has-backend macro to consumers (App, tests). Match the naming
# convention used by the multi-backend runtime selection (plan §13 / M2)
# and XEngineVulkanRHI's XENGINE_HAS_VULKAN_BACKEND.
# INTERFACE means consumers (anything that PUBLIC-links this target) see it.
target_compile_definitions(XEngineD3D12RHI
    INTERFACE
        XENGINE_HAS_D3D12_BACKEND
)

# Optional D3D12 + DXGI + WRL wiring. Gated by WIN32 — the parent CMake
# only enters this subdirectory on WIN32 platforms. We still guard here
# defensively in case someone includes this file from a different scope.
if(WIN32)
    # D3D12 / DXGI are reached through the Windows SDK that ships with
    # every supported Visual Studio / Windows 10+ toolchain. We link the
    # libs by name — Visual Studio's default library search paths already
    # include the Windows SDK lib directories. No find_package is required.
    target_link_libraries(XEngineD3D12RHI
        PRIVATE
            d3d12
            dxgi
            dxguid
    )

    # Ensure Unicode-aware Win32 APIs (WideCharToMultiByte uses CP_UTF8).
    target_compile_definitions(XEngineD3D12RHI
        PRIVATE
            UNICODE
            _UNICODE
    )
endif()