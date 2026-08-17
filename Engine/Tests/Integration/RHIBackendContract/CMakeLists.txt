# Integration/RHIBackendContract tests
#
# Backend-parameterized contract tests. A single set of TEST_P bodies runs
# against every backend that's actually linked into this target. The
# backend list is hard-coded in the test source (Vulkan / D3D12 / Metal)
# and the fixture GTEST_SKIP()s each PARAM value that isn't registered
# with RHIRuntime — so the same binary works whether you build with
# Vulkan only, D3D12 only, or both.
#
# This is intentionally a separate target from XEngineRHIIntegrationTests
# (the build probe in Integration/RHI/). Putting these sources into the
# same Source/ tree would re-include them in a target that disallows
# XEngineVulkanRHI / XEngineD3D12RHI links (see Integration/RHI/CMakeLists.txt
# "Disallowed dependencies") and fail to link.
#
# Allowed dependencies:
#   XEngineFoundation, XEngineRHI, XEngineTestSupport, GTest.
#   XEngineVulkanRHI / XEngineD3D12RHI conditionally when their targets
#   exist (this target opts in via if(TARGET ...) below).
#
# Disallowed dependencies: Renderer, Editor, Sandbox, Asset,
# XEngineCoreRuntime, XEngineShader, XEnginePlatform.

# Probe available backend targets; link only the ones that exist.
set(_contract_backend_libs XEngineFoundation XEngineRHI XEngineTestSupport)
set(_contract_backend_defines)

if(TARGET XEngineVulkanRHI)
    list(APPEND _contract_backend_libs XEngineVulkanRHI)
    list(APPEND _contract_backend_defines "XENGINE_HAS_VULKAN_BACKEND=1")
endif()

if(TARGET XEngineD3D12RHI)
    list(APPEND _contract_backend_libs XEngineD3D12RHI)
    list(APPEND _contract_backend_defines "XENGINE_HAS_D3D12_BACKEND=1")
endif()

file(GLOB_RECURSE XENGINE_RHI_BACKEND_CONTRACT_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
)

add_executable(XEngineRHIIntegrationContractTests
    ${XENGINE_RHI_BACKEND_CONTRACT_SOURCES}
)

target_include_directories(XEngineRHIIntegrationContractTests
    PRIVATE
        Source
        ../../TestSupport/Public
)

target_compile_features(XEngineRHIIntegrationContractTests PRIVATE cxx_std_20)

target_link_libraries(XEngineRHIIntegrationContractTests
    PRIVATE
        ${_contract_backend_libs}
        GTest::gtest
        GTest::gtest_main
)

# XENGINE_HAS_*_BACKEND are used by the test source to hard-code which
# backend enum values to include in the INSTANTIATE_TEST_SUITE_P list.
# Runtime skipping inside SetUp() also checks RHIRuntime::EnumerateBackends().
if(_contract_backend_defines)
    target_compile_definitions(XEngineRHIIntegrationContractTests
        PRIVATE
            ${_contract_backend_defines}
    )
endif()

gtest_discover_tests(XEngineRHIIntegrationContractTests
    DISCOVERY_TIMEOUT 30
)