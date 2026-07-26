// Skeleton Vulkan RHI integration test.
//
// This file exists to prove the integration test target compiles and links
// against XEngineRHI (which currently contains the Vulkan backend) and the
// Vulkan SDK. It intentionally does NOT create a VkInstance, VkPhysicalDevice,
// VkDevice, or VkCommandBuffer yet, because:
//
//   1. XEngineVulkanRHI is not a separate target yet. The public surface is
//      still XEngineRHI's internal Vulkan code path, which is not designed
//      for direct use by tests.
//   2. Refactoring RHI is out of scope for this task per prompts.md.
//
// Once XEngineVulkanRHI is split out and exposes a stable initialization
// entry point, this file is the right place to add:
//   * VkInstance creation through the RHI loader
//   * Physical device selection
//   * Logical device creation
//   * Command pool / command buffer / queue family contract checks
//   * Resource creation (buffer, texture, sampler)
//   * GPU memory readback helpers

#include <gtest/gtest.h>

#include <XEngine/Test/TestSupport.h>

TEST(VulkanRHIIntegration, SkeletonBuildsAndLinks)
{
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
}