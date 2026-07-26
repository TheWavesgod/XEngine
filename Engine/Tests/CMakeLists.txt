# Tests module
#
# Top-level entry point for XEngine test targets.
# Each test target lives under Engine/Tests/<Layer>/<Area>.
#
# Layer rules (enforced by CMake dependency graph, not source-level checks):
#   Unit        -> XEngineCore / XEngineRHI / TestSupport / GoogleTest
#   Integration -> XEngineCore / XEngineRHI / Vulkan SDK / TestSupport / GoogleTest
#                   (XEngineVulkanRHI will be added here when that target exists)
#   Smoke       -> XEngineCore / XEnginePlatform / XEngineRHI / Vulkan SDK / SDL3
#                   (XEngineVulkanRHI will be added here when that target exists)
#
# Runtime modules must never depend on Tests / TestSupport.

option(XENGINE_BUILD_TESTS          "Build XEngine tests"                ON)
option(XENGINE_BUILD_UNIT_TESTS     "Build XEngine unit tests"           ON)
option(XENGINE_BUILD_INTEGRATION_TESTS "Build XEngine integration tests" ON)
option(XENGINE_BUILD_SMOKE_TESTS    "Build XEngine smoke tests"          ON)
option(XENGINE_BUILD_VULKAN_TESTS   "Build Vulkan-related tests"         ON)

if(NOT XENGINE_BUILD_TESTS)
    return()
endif()

include(CTest)
enable_testing()

# ----------------------------------------------------------------------------
# GoogleTest
# ----------------------------------------------------------------------------
# Pinned to a specific release tag. Do NOT switch to a floating branch.
set(XENGINE_GOOGLETEST_VERSION "v1.15.2" CACHE STRING "GoogleTest release tag")

set(XENGINE_GOOGLETEST_OPTIONS
    INSTALL_GTEST              OFF
    BUILD_GMOCK                ON
    gtest_force_shared_crt     ON   # MSVC: match MD/MDd runtime used by XEngine
    CMAKE_CXX_STANDARD         20
)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        ${XENGINE_GOOGLETEST_VERSION}
    GIT_SHALLOW    TRUE
)

# Try to use a pre-existing GoogleTest install first; fall back to fetch.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    FetchContent_MakeAvailable(googletest)
else()
    message(STATUS "Using pre-installed GoogleTest (version unknown)")
endif()

# Some FetchContent builds do not auto-create the GTest::gtest alias.
if(NOT TARGET GTest::gtest)
    if(TARGET gtest)
        add_library(GTest::gtest ALIAS gtest)
    elseif(TARGET gtest_main)
        add_library(GTest::gtest ALIAS gtest_main)
    endif()
endif()

# ----------------------------------------------------------------------------
# Layer entry points
# ----------------------------------------------------------------------------
add_subdirectory(TestSupport)

if(XENGINE_BUILD_UNIT_TESTS AND XENGINE_BUILD_VULKAN_TESTS)
    add_subdirectory(Unit)
endif()

if(XENGINE_BUILD_INTEGRATION_TESTS)
    add_subdirectory(Integration)
endif()

if(XENGINE_BUILD_SMOKE_TESTS AND XENGINE_BUILD_VULKAN_TESTS)
    add_subdirectory(Smoke)
endif()