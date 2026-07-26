# Unit/RHI tests
#
# Pure-logic tests that must NOT depend on the Vulkan SDK, SDL3, or any
# runtime component (Renderer, Asset, Editor, Sandbox). Allowed: XEngineCore
# (Foundation + CoreRuntime), XEngineRHI, XEngineTestSupport, GoogleTest.
#
# Note: XEngineRHI is the backend-agnostic public interface only. Backend
# code paths must NOT be exercised here. The unit test below currently only
# verifies that the test framework links against XEngineRHI and that the
# public type forward-declarations resolve.

file(GLOB_RECURSE XENGINE_RHI_UNIT_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
)

add_executable(XEngineRHIUnitTests ${XENGINE_RHI_UNIT_SOURCES})

target_include_directories(XEngineRHIUnitTests
    PRIVATE
        Source
        ../TestSupport/Public
)

target_compile_features(XEngineRHIUnitTests PRIVATE cxx_std_20)
target_link_libraries(XEngineRHIUnitTests
    PRIVATE
        XEngineFoundation
        XEngineCoreRuntime
        XEngineRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

# Register every TEST() in the executable as a CTest case.
gtest_discover_tests(XEngineRHIUnitTests
    DISCOVERY_TIMEOUT 30
)