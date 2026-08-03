# Unit tests layer
#
# Unit tests run without a real GPU, window system, or specific graphics API.
# Allowed dependencies:
#   XEngineFoundation
#   XEngineRHI
#   XEngineTestSupport
#   GoogleTest
#
# Disallowed dependencies:
#   Vulkan SDK, XEngineVulkanRHI, SDL3, Renderer, Asset, Editor, Sandbox,
#   XEngineCoreRuntime, XEngineShader, XEnginePlatform, XEngineEngine.

add_subdirectory(RHI)
