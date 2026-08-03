// Unit tests for RHICapabilities struct.
//
// RHICapabilities is a POD-style data carrier. Tests verify field defaults,
// assignment, and ABI / size stability. M3 establishes the baseline; M4+
// extends with new fields.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIDevice.h>  // RHICapabilities

#include <type_traits>

namespace
{
    using namespace XEngine;

    // ABI-size stability: M3 must keep its size backward-compatible.
    // If M4+ adds a field, document it but don't shrink.

    TEST(RHICapabilities, DefaultsAreZeroOrConservative)
    {
        RHICapabilities caps;

        // Numeric limits default to 0 (unset / unknown)
        EXPECT_EQ(caps.MaxTextureSize2D, 0u);
        EXPECT_EQ(caps.MaxBufferSize, 0u);
        EXPECT_EQ(caps.MaxSamplerAnisotropy, 0u);
        EXPECT_EQ(caps.MaxSampleCount, 0u);
        EXPECT_EQ(caps.MaxViewports, 1u);        // 1 is the minimum
        EXPECT_EQ(caps.MaxColorAttachments, 0u);
        EXPECT_EQ(caps.MaxFramesInFlight, 1u);   // 1 is the minimum
        EXPECT_EQ(caps.MaxComputeWorkGroupInvocations, 0u);
        EXPECT_EQ(caps.MaxComputeSharedMemorySize, 0u);

        // Alignment defaults to 1 (no restriction)
        EXPECT_EQ(caps.MinUniformBufferOffsetAlignment, 1u);
        EXPECT_EQ(caps.MinStorageBufferOffsetAlignment, 1u);
        EXPECT_EQ(caps.MinTexelBufferOffsetAlignment, 1u);

        // Features: depth clip is the only one with true default (D3D12 baseline)
        EXPECT_TRUE(caps.SupportsDepthClip);
        EXPECT_FALSE(caps.SupportsDepthBiasClamp);
        EXPECT_FALSE(caps.SupportsWideLines);
        EXPECT_FALSE(caps.SupportsLargePoints);
        EXPECT_FALSE(caps.SupportsTimelineSemaphore);
        EXPECT_FALSE(caps.SupportsPushDescriptor);
        EXPECT_FALSE(caps.SupportsBindless);
        EXPECT_FALSE(caps.SupportsBufferDeviceAddress);
        EXPECT_FALSE(caps.SupportsRayTracing);
        EXPECT_FALSE(caps.SupportsGeometryShader);
        EXPECT_FALSE(caps.SupportsTessellationShader);
    }

    TEST(RHICapabilities, IsAssignable)
    {
        RHICapabilities caps;
        caps.MaxTextureSize2D                  = 8192;
        caps.MaxBufferSize                     = 1ull << 30;
        caps.MaxFramesInFlight                 = 3;
        caps.SupportsTimelineSemaphore         = true;
        caps.SupportsPushDescriptor            = true;
        caps.SupportsBufferDeviceAddress       = true;
        caps.MinUniformBufferOffsetAlignment   = 256;
        caps.MinStorageBufferOffsetAlignment   = 16;

        EXPECT_EQ(caps.MaxTextureSize2D, 8192u);
        EXPECT_EQ(caps.MaxBufferSize, 1ull << 30);
        EXPECT_EQ(caps.MaxFramesInFlight, 3u);
        EXPECT_TRUE(caps.SupportsTimelineSemaphore);
        EXPECT_TRUE(caps.SupportsPushDescriptor);
        EXPECT_TRUE(caps.SupportsBufferDeviceAddress);
        EXPECT_EQ(caps.MinUniformBufferOffsetAlignment, 256u);
        EXPECT_EQ(caps.MinStorageBufferOffsetAlignment, 16u);
    }

    TEST(RHICapabilities, BackendCanConfigureVulkanLikeProfile)
    {
        // Simulates a typical Vulkan 1.2 GPU's capabilities.
        RHICapabilities caps;
        caps.MaxTextureSize2D                = 16384;
        caps.MaxBufferSize                   = 1ull << 30;
        caps.MaxSamplerAnisotropy            = 16;
        caps.MaxSampleCount                  = 8;
        caps.MaxViewports                    = 16;
        caps.MaxColorAttachments             = 8;
        caps.MaxFramesInFlight               = 2;
        caps.MaxComputeWorkGroupInvocations  = 1024;
        caps.MaxComputeSharedMemorySize      = 49152;
        caps.MinUniformBufferOffsetAlignment = 256;
        caps.MinStorageBufferOffsetAlignment = 16;
        caps.MinTexelBufferOffsetAlignment    = 16;
        caps.SupportsDepthClip               = true;
        caps.SupportsDepthBiasClamp          = true;
        caps.SupportsWideLines               = true;
        caps.SupportsTimelineSemaphore       = true;
        caps.SupportsPushDescriptor          = true;
        caps.SupportsBufferDeviceAddress     = true;

        EXPECT_EQ(caps.MaxTextureSize2D, 16384u);
        EXPECT_TRUE(caps.SupportsTimelineSemaphore);
        EXPECT_TRUE(caps.SupportsPushDescriptor);
    }

    TEST(RHICapabilities, BackendCanConfigureD3D12LikeProfile)
    {
        // Simulates a D3D12-style GPU profile.
        RHICapabilities caps;
        caps.MaxTextureSize2D = 16384;
        caps.MaxBufferSize    = 1ull << 30;
        caps.MaxSampleCount   = 8;
        caps.MaxFramesInFlight = 3;
        caps.SupportsDepthClip = true;
        caps.SupportsRayTracing = true;  // D3D12 has DXR

        EXPECT_EQ(caps.MaxFramesInFlight, 3u);
        EXPECT_TRUE(caps.SupportsRayTracing);
    }
}
