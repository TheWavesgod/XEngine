# Integration/VulkanRHI tests
#
# Per-backend integration tests for the Vulkan RHI. After the rebuild these
# tests prove that XEngineVulkanRHI compiles, links, and exposes its public
# surface to a consumer. The Vulkan SDK and volk are reached only through
# XEngineVulkanRHI's PRIVATE link surface, so this test target does NOT
# need to call find_package(Vulkan) itself.
#
# Once XEngineVulkanRHI exposes a stable Vulkan initialization entry point,
# this file is the right place to add:
#   * VkInstance creation through the RHI loader
#   * Physical device selection
#   * Logical device creation
#   * Command pool / command buffer / queue-family contract checks
#   * Resource creation (buffer, texture, sampler)
#   * GPU memory readback helpers
#
# Allowed dependencies:
#   XEngineFoundation, XEngineRHI, XEngineVulkanRHI, XEngineTestSupport,
#   GoogleTest.
#
# Disallowed: XEngineCoreRuntime, Renderer, Editor, Sandbox, Asset,
# direct Vulkan SDK headers / volk / VMA in this CMake file.

file(GLOB_RECURSE XENGINE_VULKAN_RHI_INTEGRATION_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
)

add_executable(XEngineVulkanRHIIntegrationTests ${XENGINE_VULKAN_RHI_INTEGRATION_SOURCES})

target_include_directories(XEngineVulkanRHIIntegrationTests
    PRIVATE
        Source
        ../../TestSupport/Public
)

target_compile_features(XEngineVulkanRHIIntegrationTests PRIVATE cxx_std_20)

target_link_libraries(XEngineVulkanRHIIntegrationTests
    PRIVATE
        XEngineFoundation
        XEngineRHI
        XEngineVulkanRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(XEngineVulkanRHIIntegrationTests
    DISCOVERY_TIMEOUT 30
)
