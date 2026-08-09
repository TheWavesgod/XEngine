# Integration/D3D12RHI tests
#
# Per-backend integration tests for the D3D12 RHI. After the rebuild these
# tests prove that XEngineD3D12RHI compiles, links, and exposes its public
# surface to a consumer. The d3d12 / dxgi / WRL APIs are reached only
# through XEngineD3D12RHI's PRIVATE link surface, so this test target
# does NOT need to include <d3d12.h> / <dxgi1_4.h> / <wrl/client.h>
# directly.
#
# Once XEngineD3D12RHI exposes a stable ID3D12Device creation entry point,
# this file is the right place to add:
#   * IDXGIFactory4 / IDXGIAdapter1 creation through the RHI loader
#   * ID3D12Device creation with various feature levels
#   * Command queue / command list / command allocator contract checks
#   * Resource creation (buffer, texture, sampler, descriptor heap)
#   * GPU memory readback helpers
#
# Allowed dependencies:
#   XEngineFoundation, XEngineRHI, XEngineD3D12RHI, XEngineTestSupport,
#   GoogleTest.
#
# Disallowed: XEngineCoreRuntime, Renderer, Editor, Sandbox, Asset,
# direct d3d12.h / dxgi1_4.h / wrl/client.h includes in this CMake file.

file(GLOB_RECURSE XENGINE_D3D12_RHI_INTEGRATION_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
)

add_executable(XEngineD3D12RHIIntegrationTests ${XENGINE_D3D12_RHI_INTEGRATION_SOURCES})

target_include_directories(XEngineD3D12RHIIntegrationTests
    PRIVATE
        Source
        ../../TestSupport/Public
)

target_compile_features(XEngineD3D12RHIIntegrationTests PRIVATE cxx_std_20)

target_link_libraries(XEngineD3D12RHIIntegrationTests
    PRIVATE
        XEngineFoundation
        XEngineRHI
        XEngineD3D12RHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
)

# When XEngineVulkanRHI is also built, link it too so the multi-backend
# registry coexistence test (D3D12RHISkeleton.MultiBackendRegistryCoexistsWithVulkan)
# can register both backends from the same process. Without this link,
# the Vulkan-only test case is compiled out via the XENGINE_HAS_VULKAN_BACKEND
# guard.
if(TARGET XEngineVulkanRHI)
    target_link_libraries(XEngineD3D12RHIIntegrationTests
        PRIVATE
            XEngineVulkanRHI
    )
endif()

gtest_discover_tests(XEngineD3D12RHIIntegrationTests
    DISCOVERY_TIMEOUT 30
)