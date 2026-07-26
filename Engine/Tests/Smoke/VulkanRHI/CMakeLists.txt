# Smoke/VulkanRHI executable skeleton.
#
# This is the place where the future "open a window, init Vulkan, clear, swap,
# exit" smoke test will live. It does NOT use GoogleTest -- it is a standalone
# executable registered with CTest.
#
# CURRENT STATE (RHI refactor in progress):
#   * XEngineVulkanRHI does not exist as a separate target yet.
#   * XEnginePlatform currently links Vulkan + SDL3 as a side effect of
#     XEngineRHI's CMake, which means linking XEnginePlatform here can drag
#     in Vulkan types not yet isolated to a VulkanRHI target.
#
# To keep this commit minimal and not "fix" link issues by hacking
# XEnginePlatform / XEngineRHI, this skeleton target currently only links
# against XEngineFoundation / XEngineCoreRuntime and the Vulkan SDK headers
# -- no XEnginePlatform, no SDL3, no XEngineRHI -- and its source only
# verifies that the executable itself links. Real smoke logic will be added
# when XEngineVulkanRHI and a stable platform initialization entry exist.

find_package(Vulkan REQUIRED)

add_executable(XEngineVulkanRHISmokeTest
    Source/main.cpp
)

target_include_directories(XEngineVulkanRHISmokeTest
    PRIVATE
        Source
        ${Vulkan_INCLUDE_DIRS}
)

target_compile_features(XEngineVulkanRHISmokeTest PRIVATE cxx_std_20)
target_compile_definitions(XEngineVulkanRHISmokeTest
    PRIVATE
        VK_NO_PROTOTYPES
)

target_link_libraries(XEngineVulkanRHISmokeTest
    PRIVATE
        XEngineFoundation
        XEngineCoreRuntime
        ${Vulkan_LIBRARIES}
)

add_test(
    NAME XEngineVulkanRHISmokeTest.SkeletonLinks
    COMMAND XEngineVulkanRHISmokeTest
)