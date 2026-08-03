// Skeleton Vulkan RHI integration test for the new XEngineVulkanRHI target.
//
// This file exists to prove the integration test target compiles and links
// against the new XEngineVulkanRHI target (which is now a separate, minimal
// skeleton). It intentionally does NOT create a VkInstance, VkPhysicalDevice,
// VkDevice, or VkCommandBuffer yet, because per Docs/AI helper/prompts.md:
//
//   1. The new XEngineVulkanRHI public surface is currently empty (only a
//      version probe) — the RHIInstance / RHIAdapter / RHIDevice API
//      design has deliberately not started yet.
//   2. Designing or implementing any of those protocols is out of scope
//      for this task. Once they land, this file is where the new
//      contract checks will live.
//
// What this file DOES verify today:
//   * XEngineVulkanRHI's public header (XEngine/VulkanRHI/Base.h) is
//     visible from the test executable.
//   * XEngineRHI's public header (XEngine/RHI/Base.h) is still visible
//     (it is a PUBLIC dep of XEngineVulkanRHI).
//   * The version probe defined in XEngineVulkanRHI/Private/VulkanRHI.cpp
//     resolves at link time.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/Test/TestSupport.h>
#include <XEngine/VulkanRHI/Base.h>

#include <cstdint>

namespace XEngine
{
    namespace
    {
        static_assert(sizeof(XEngine::VulkanRHIVersionMajor) == sizeof(std::uint32_t),
                      "RHI version sentinel must be 32-bit.");
    } // namespace
} // namespace XEngine

TEST(VulkanRHIIntegration, SkeletonBuildsAndLinks)
{
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
}

TEST(VulkanRHIIntegration, NewVulkanRHIVersionTriplesExposed)
{
    // Link-time resolution test: if XEngineVulkanRHI is not actually
    // linked, the linker will fail. The runtime checks below produce a
    // useful CTest output entry in case the linkers succeed but values
    // ever drift.
    EXPECT_EQ(XEngineVulkanRHI_GetVersionMajor(),
              XEngine::VulkanRHIVersionMajor);
    EXPECT_EQ(XEngineVulkanRHI_GetVersionMinor(),
              XEngine::VulkanRHIVersionMinor);
    EXPECT_EQ(XEngineVulkanRHI_GetVersionPatch(),
              XEngine::VulkanRHIVersionPatch);
}
