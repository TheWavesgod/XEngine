# Integration tests layer
#
# Post-RHI-rebuild layout. Per Docs/AI helper/prompts.md:
#
#   RHI        -> backend-agnostic contract tests, links XEngineRHI only.
#   VulkanRHI  -> Vulkan-specific integration tests, links XEngineRHI +
#                 XEngineVulkanRHI. Only available when both RHI and
#                 VulkanRHI are enabled.
#   D3D12RHI   -> D3D12-specific integration tests, links XEngineRHI +
#                 XEngineD3D12RHI. Only available when both RHI and
#                 D3D12RHI are enabled (WIN32 only).
#
# Allowed dependencies:
#   XEngineFoundation
#   XEngineRHI / XEngineVulkanRHI / XEngineD3D12RHI
#   XEngineTestSupport
#   Vulkan SDK (via XEngineVulkanRHI PRIVATE link, only when the Vulkan
#              test wants to talk to a real VkInstance)
#   GoogleTest
#
# Disallowed dependencies: Renderer, Editor, Sandbox, Asset, XEngineCoreRuntime,
# XEngineShader, XEnginePlatform, XEngineEngine.

add_subdirectory(RHI)

if(TARGET XEngineVulkanRHI)
    add_subdirectory(VulkanRHI)
endif()

if(TARGET XEngineD3D12RHI)
    add_subdirectory(D3D12RHI)
endif()
