# Smoke/VulkanRHI executable skeleton.
#
# Per Docs/AI helper/prompts.md the smoke test's required dependency list is:
#
#   XEngineVulkanRHI  (PUBLIC)
#   minimal Platform/Window dependency (NOT YET LINKED — see below)
#
# SDL3 is permitted here only in the future window-creation path; today no
# window is created and no SDL3 link is performed. Vulkan SDK / volk / VMA
# are reached ONLY through XEngineVulkanRHI's PRIVATE link surface, so this
# CMake file does not call find_package(Vulkan).
#
# This is the place where the future "open a window, init Vulkan, clear,
# swap, exit" smoke test will live. Until XEngineVulkanRHI exposes a
# stable initialization entry point the executable only verifies that:
#
#   * XEngineVulkanRHI links correctly.
#   * XEngineVulkanRHI's public header (XEngine/VulkanRHI/Base.h) is
#     visible.
#   * The version probe defined in XEngineVulkanRHI/Private/VulkanRHI.cpp
#     resolves at link time.

add_executable(XEngineVulkanRHISmokeTest
    Source/main.cpp
)

target_include_directories(XEngineVulkanRHISmokeTest
    PRIVATE
        Source
        ../../TestSupport/Public
)

target_compile_features(XEngineVulkanRHISmokeTest PRIVATE cxx_std_20)
target_link_libraries(XEngineVulkanRHISmokeTest
    PRIVATE
        XEngineFoundation
        XEngineRHI
        XEngineVulkanRHI
        XEngineTestSupport
)

add_test(
    NAME XEngineVulkanRHISmokeTest.SkeletonLinks
    COMMAND XEngineVulkanRHISmokeTest
)
