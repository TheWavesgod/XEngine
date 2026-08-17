# Tests module
#
# Top-level entry point for XEngine test targets. Each test target lives under
# Engine/Tests/<Layer>/<Area>.
#
# Post-RHI-rebuild layer rules (enforced by CMake dependency graph, not
# source-level checks):
#
#   Unit        -> XEngineFoundation / XEngineRHI / XEngineTestSupport / GTest
#   Integration -> XEngineFoundation / XEngineRHI / XEngineVulkanRHI / Vulkan SDK / TestSupport / GTest
#   Smoke       -> XEngineFoundation / XEngineVulkanRHI / Vulkan SDK / SDL3 (window only) / TestSupport
#
# Runtime modules (Renderer, Editor, Asset, Scene, Engine runtime,
# Shader compiler, ...) must never depend on Tests / TestSupport.
#
# A test is allowed to link XEngineVulkanRHI only when XEngineVulkanRHI is
# actually being built via XENGINE_BUILD_VULKAN_RHI=ON.

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

# Unit layer is backend-agnostic and must NOT depend on Vulkan SDK or
# XEngineVulkanRHI. The XENGINE_BUILD_VULKAN_TESTS gate is kept for backwards
# compatibility with the previous tree; it does not currently gate anything in
# Unit/ after the rebuild.
if(XENGINE_BUILD_UNIT_TESTS AND TARGET XEngineRHI)
    add_subdirectory(Unit)
endif()

# Integration layer is split into:
#   RHI              -> backend-agnostic build probe, links XEngineRHI only.
#   RHIBackendContract -> backend-parameterized contract tests; the same
#                         test body runs against every backend that's
#                         actually linked (Vulkan / D3D12 / Metal). Lives
#                         in its own subdirectory because it links backend
#                         targets — the Integration/RHI target above
#                         disallows them.
#   VulkanRHI        -> Vulkan-specific integration tests, links XEngineRHI +
#                       XEngineVulkanRHI. Only available when both RHI and VulkanRHI
#                       are enabled.
#   D3D12RHI         -> D3D12-specific integration tests, links XEngineRHI +
#                       XEngineD3D12RHI. Only available when both RHI and D3D12RHI
#                       are enabled (D3D12RHI is gated to WIN32 by its own
#                       CMakeLists).
if(XENGINE_BUILD_INTEGRATION_TESTS)
    if(TARGET XEngineRHI)
        add_subdirectory(Integration/RHI)
    endif()
    # Backend-parameterized contract tests need at least one backend
    # to exercise. If neither Vulkan nor D3D12 is built the target
    # will have nothing to test and gtest_discover_tests expands to
    # only the skipped Metal case — still useful as a build probe.
    if(TARGET XEngineRHI AND (TARGET XEngineVulkanRHI OR TARGET XEngineD3D12RHI))
        add_subdirectory(Integration/RHIBackendContract)
    endif()
    if(TARGET XEngineRHI AND TARGET XEngineVulkanRHI AND XENGINE_BUILD_VULKAN_TESTS)
        add_subdirectory(Integration/VulkanRHI)
    endif()
    if(TARGET XEngineRHI AND TARGET XEngineD3D12RHI)
        add_subdirectory(Integration/D3D12RHI)
    endif()
endif()

# Smoke layer is Vulkan-only at this stage. Per prompts.md the smoke test must
# be able to construct a minimal Platform/Window dependency, but it does not
# need to do so today.
if(XENGINE_BUILD_SMOKE_TESTS AND XENGINE_BUILD_VULKAN_TESTS AND TARGET XEngineVulkanRHI)
    add_subdirectory(Smoke)
endif()
