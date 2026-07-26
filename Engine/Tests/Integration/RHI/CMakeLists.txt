# Integration/RHI tests
#
# Cross-backend RHI contract tests. These tests exercise the public RHI
# surface through a real backend (currently Vulkan via XEngineRHI). They are
# the right place to assert invariants that should hold for every backend,
# such as resource handle validity and submission ordering.
#
# Allowed dependencies:
#   XEngineFoundation, XEngineCoreRuntime, XEngineRHI, XEngineTestSupport,
#   GoogleTest, Vulkan SDK.
#
# Disallowed: Renderer, Editor, Sandbox, Asset.

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
        XEngineCoreRuntime
        XEngineRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(XEngineRHIIntegrationTests
    DISCOVERY_TIMEOUT 30
)