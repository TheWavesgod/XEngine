# Unit tests layer
#
# Unit tests run without a real GPU, window system, or specific graphics API.
# Allowed dependencies:
#   XEngineFoundation / XEngineCoreRuntime (XEngineCore)
#   XEngineRHI
#   XEngineTestSupport
#   GoogleTest
#
# Disallowed dependencies:
#   Vulkan SDK, XEngineVulkanRHI, SDL3, Renderer, Asset, Editor, Sandbox

add_subdirectory(RHI)