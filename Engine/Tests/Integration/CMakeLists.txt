# Integration tests layer
#
# Integration tests combine multiple real components. They may link against
# the Vulkan SDK and (in the future) XEngineVulkanRHI once that target exists.
# Currently, backend-specific Vulkan code still lives inside XEngineRHI itself,
# so the Vulkan integration tests link against XEngineRHI + Vulkan SDK.
#
# Allowed dependencies:
#   XEngineCore (Foundation / CoreRuntime)
#   XEngineRHI
#   XEngineTestSupport
#   Vulkan SDK
#   GoogleTest
#   (XEngineVulkanRHI -- added when that target exists)
#
# Disallowed dependencies: Renderer, Editor, Sandbox, Asset.

add_subdirectory(RHI)

if(XENGINE_ENABLE_VULKAN)
    add_subdirectory(VulkanRHI)
endif()