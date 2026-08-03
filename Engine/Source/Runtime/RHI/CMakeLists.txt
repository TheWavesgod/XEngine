# New XEngineRHI target — minimal skeleton only.
#
# Per Docs/AI helper/prompts.md, this target establishes the boundary for
# the rebuilt RHI. It is intentionally minimal:
#
#   * Public header surface: XEngine/RHI/Base.h (namespace, export macro,
#     Result alias, RHIBackend tag, version sentinel).
#   * Private translation unit: RHI.cpp (version queries).
#
# XEngineRHI MUST NOT depend on Vulkan, volk, VMA, SDL3, Shader compiler,
# Renderer, Editor, Asset, Scene, or any other runtime module beyond the
# minimal XEngineFoundation layer. Future protocol additions
# (RHIInstance, RHIAdapter, RHIDevice, RHIQueue, RHIBuffer, RHITexture,
# RHICommandList, RHIPipeline, RHISwapchain, …) will land in later
# stages of this refactor; nothing here pre-defines those types.
#
# Hard rules enforced by review:
#   - No Vulkan headers / volk / VMA / SDL3 in this target.
#   - No XEngineVulkanRHI dependency.
#   - No Shader / Renderer / Asset / Scene / Editor / CoreRuntime link.
#   - No vulkan-specific compile definitions (XENGINE_ENABLE_VULKAN,
#     VK_NO_PROTOTYPES, …) on this target.

file(GLOB_RECURSE XENGINE_RHI_SOURCES
    CONFIGURE_DEPENDS
        Public/*.h
        Private/*.cpp
)

add_library(XEngineRHI ${XENGINE_RHI_SOURCES})

target_include_directories(XEngineRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(XEngineRHI PUBLIC cxx_std_20)

# The new XEngineRHI is currently a static library, so its export macro
# is a no-op. Define XENGINE_RHI_STATIC when building this target so the
# XENGINE_RHI_API tokens in Export.h resolve to empty.
target_compile_definitions(XEngineRHI
    PUBLIC
        XENGINE_RHI_STATIC
)

target_link_libraries(XEngineRHI
    PUBLIC
        XEngineFoundation
)

# XEngineRHI is currently consumed only by XEngineVulkanRHI and by tests.
# It is intentionally NOT added to any umbrella "Runtime" target because
# the rest of the engine (Renderer / Editor / Asset / Scene / Engine
# runtime) is temporarily frozen per prompts.md.
