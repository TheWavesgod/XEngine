// Phase 1 (M0-M3 backend) Vulkan RHI integration test.
//
// Verifies the new XEngineVulkanRHI against a real GPU / loader:
//   * VkInstance creation through VulkanInstance::CreateInstance
//   * Physical device enumeration
//   * Logical device creation
//   * Per-family queue retrieval
//   * Capabilities population (M3 audit)
//
// Phase 2+ will add buffer roundtrip and fence/semaphore tests.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIAdapter.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/Test/TestSupport.h>
#include <XEngine/VulkanRHI/Base.h>
#include <XEngine/VulkanRHI/Backend.h>

#include <cstdint>
#include <memory>

namespace XEngine
{
    namespace
    {
        static_assert(sizeof(XEngine::VulkanRHIVersionMajor) == sizeof(std::uint32_t),
                      "RHI version sentinel must be 32-bit.");
    } // namespace
} // namespace XEngine

TEST(VulkanRHIIntegration, LinkTimeProbeResolves)
{
    // If XEngineVulkanRHI is not actually linked, the linker would fail.
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
    EXPECT_EQ(XEngineVulkanRHI_GetVersionMajor(), XEngine::VulkanRHIVersionMajor);
    EXPECT_EQ(XEngineVulkanRHI_GetVersionMinor(), XEngine::VulkanRHIVersionMinor);
    EXPECT_EQ(XEngineVulkanRHI_GetVersionPatch(), XEngine::VulkanRHIVersionPatch);
}

TEST(VulkanRHIIntegration, CreateInstanceSucceeds)
{
    XEngine::RHIInstanceDesc desc;
    desc.ApplicationName = "VulkanRHIIntegrationTest";
    desc.ApplicationVersion = 0;
    desc.EnableValidation = false;
    desc.EnableDebugMarkers = true;

    auto instance = XEngine::VulkanRHI::CreateInstance(desc);
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->GetBackend(), XEngine::RHIBackend::Vulkan);
}

TEST(VulkanRHIIntegration, EnumerateAdaptersReturnsAtLeastOne)
{
    auto instance = XEngine::VulkanRHI::CreateInstance({});
    ASSERT_NE(instance, nullptr);

    auto adapters = instance->EnumerateAdapters();
    EXPECT_GE(adapters.size(), 1u);
    for (auto& a : adapters)
    {
        const auto info = a->GetInfo();
        EXPECT_FALSE(info.VendorName.empty());
        EXPECT_FALSE(info.AdapterName.empty());
        EXPECT_NE(info.Type, XEngine::RHIAdapterType::Unknown);
    }
}

TEST(VulkanRHIIntegration, CreateDeviceAndGetQueues)
{
    auto instance = XEngine::VulkanRHI::CreateInstance({});
    ASSERT_NE(instance, nullptr);

    auto adapters = instance->EnumerateAdapters();
    ASSERT_FALSE(adapters.empty());

    auto& adapter = *adapters.front();
    auto* device = instance->CreateDevice(adapter, {});
    ASSERT_NE(device, nullptr);

    EXPECT_EQ(device->GetBackend(), XEngine::RHIBackend::Vulkan);
    EXPECT_EQ(device->GetMaxFramesInFlight(), 2u);

    const auto& caps = device->GetCapabilities();
    EXPECT_GT(caps.MaxTextureSize2D, 0u);
    EXPECT_GT(caps.MaxSamplerAnisotropy, 0u);

    auto* gfx = device->GetQueue(XEngine::RHIQueueType::Graphics);
    auto* cmp = device->GetQueue(XEngine::RHIQueueType::Compute);
    auto* xfr = device->GetQueue(XEngine::RHIQueueType::Transfer);
    EXPECT_NE(gfx, nullptr);
    EXPECT_EQ(gfx->GetType(), XEngine::RHIQueueType::Graphics);
    EXPECT_NE(cmp, nullptr);
    EXPECT_EQ(cmp->GetType(), XEngine::RHIQueueType::Compute);
    EXPECT_NE(xfr, nullptr);
    EXPECT_EQ(xfr->GetType(), XEngine::RHIQueueType::Transfer);

    device->WaitIdle();
}
