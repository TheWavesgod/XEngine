# Unit/RHI tests
#
# Pure-logic tests that must NOT depend on the Vulkan SDK, SDL3, or any
# runtime component (Renderer, Asset, Editor, Sandbox). Allowed:
#   XEngineFoundation, XEngineRHI, XEngineTestSupport, GoogleTest.
#
# After the RHI rebuild the public surface of XEngineRHI is intentionally
# minimal (XEngine/RHI/Base.h). The unit test below verifies that:
#
#   1. GoogleTest is correctly wired up via Tests/CMakeLists.txt.
#   2. XEngineRHI headers are visible and linkable from a unit test target.
#   3. XEngineFoundation is visible and linkable.
#   4. The version probe defined in XEngineRHI/Private/RHI.cpp resolves at
#      link time.

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
        XEngineRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

# Register every TEST() in the executable as a CTest case.
gtest_discover_tests(XEngineRHIUnitTests
    DISCOVERY_TIMEOUT 30
)
