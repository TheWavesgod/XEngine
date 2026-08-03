# Integration/RHI tests
#
# Cross-backend RHI contract tests. After the rebuild these tests only
# prove that the public surface of XEngineRHI is linkable from an
# integration test executable. They MUST NOT touch any Vulkan / volk /
# VMA / SDL3 symbols; per-backend tests live in Integration/VulkanRHI.
#
# Allowed dependencies:
#   XEngineFoundation, XEngineRHI, XEngineTestSupport, GoogleTest.
#
# Disallowed: XEngineCoreRuntime, Renderer, Editor, Sandbox, Asset,
# XEngineVulkanRHI, XEngineShader, XEnginePlatform.

file(GLOB_RECURSE XENGINE_RHI_INTEGRATION_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
)

add_executable(XEngineRHIIntegrationTests ${XENGINE_RHI_INTEGRATION_SOURCES})

target_include_directories(XEngineRHIIntegrationTests
    PRIVATE
        Source
        ../../TestSupport/Public
)

target_compile_features(XEngineRHIIntegrationTests PRIVATE cxx_std_20)
target_link_libraries(XEngineRHIIntegrationTests
    PRIVATE
        XEngineFoundation
        XEngineRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(XEngineRHIIntegrationTests
    DISCOVERY_TIMEOUT 30
)
