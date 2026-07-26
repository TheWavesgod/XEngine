# Integration/VulkanRHI tests
#
# Per-backend integration tests for the Vulkan RHI. These tests will exercise
# a real VkInstance / VkPhysicalDevice / VkDevice / VkQueue / VkCommandBuffer
# path, when the XEngineVulkanRHI separation lands.
#
# CURRENT STATE (RHI refactor in progress):
#   XEngineVulkanRHI is not yet a separate target. The Vulkan backend still
#   lives inside XEngineRHI. As a result, this target currently links only
#   against XEngineRHI + Vulkan SDK + TestSupport + GoogleTest, and the
#   source file only contains a skeleton test.
#
# Once XEngineVulkanRHI is extracted, replace XEngineRHI with XEngineVulkanRHI
# in target_link_libraries and use the XEngineVulkanRHI public surface here.

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

# Vulkan SDK is brought in by the top-level find_package(Vulkan REQUIRED) in
# Engine/CMakeLists.txt, so we can reuse Vulkan_INCLUDE_DIRS / Vulkan_LIBRARIES
# here.
find_package(Vulkan REQUIRED)

target_compile_definitions(XEngineVulkanRHIIntegrationTests
    PRIVATE
        VK_NO_PROTOTYPES
)

target_include_directories(XEngineVulkanRHIIntegrationTests
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(XEngineVulkanRHIIntegrationTests
    PRIVATE
        XEngineFoundation
        XEngineCoreRuntime
        XEngineRHI
        XEngineTestSupport
        GTest::gtest
        GTest::gtest_main
        ${Vulkan_LIBRARIES}
)

gtest_discover_tests(XEngineVulkanRHIIntegrationTests
    DISCOVERY_TIMEOUT 30
)