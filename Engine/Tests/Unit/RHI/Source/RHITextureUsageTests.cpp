// Unit tests for RHITextureUsage flag enum.
//
// Verifies enum values, ABI size, and flag operations.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHITextureUsage) == 4,
                  "RHITextureUsage must be 4 bytes (u32) for ABI stability");

    TEST(RHITextureUsage, ValuesAreContiguous)
    {
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::None), 0u);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::ShaderRead), 1u << 0);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::ShaderWrite), 1u << 1);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::RenderTarget), 1u << 2);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::DepthStencil), 1u << 3);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::TransferSrc), 1u << 4);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::TransferDst), 1u << 5);
        EXPECT_EQ(static_cast<u32>(RHITextureUsage::Present), 1u << 6);
    }

    TEST(RHITextureUsage, HasFlagSingleBit)
    {
        auto usage = RHITextureUsage::ShaderRead;
        EXPECT_TRUE(HasFlag(usage, RHITextureUsage::ShaderRead));
        EXPECT_FALSE(HasFlag(usage, RHITextureUsage::RenderTarget));
        EXPECT_FALSE(HasFlag(usage, RHITextureUsage::None));
    }

    TEST(RHITextureUsage, HasFlagMultipleBits)
    {
        auto usage = RHITextureUsage::ShaderRead | RHITextureUsage::RenderTarget;
        EXPECT_TRUE(HasFlag(usage, RHITextureUsage::ShaderRead));
        EXPECT_TRUE(HasFlag(usage, RHITextureUsage::RenderTarget));
        EXPECT_FALSE(HasFlag(usage, RHITextureUsage::DepthStencil));
    }

    TEST(RHITextureUsage, OperatorOrCombinesFlags)
    {
        auto combined = RHITextureUsage::ShaderRead | RHITextureUsage::RenderTarget;
        EXPECT_TRUE(HasFlag(combined, RHITextureUsage::ShaderRead));
        EXPECT_TRUE(HasFlag(combined, RHITextureUsage::RenderTarget));
    }

    TEST(RHITextureUsage, TypicalBackendUsage)
    {
        // Common combinations seen in real rendering pipelines.
        auto colorAttachment = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderRead;
        auto depthAttachment = RHITextureUsage::DepthStencil | RHITextureUsage::ShaderRead;
        auto storageImage = RHITextureUsage::ShaderRead | RHITextureUsage::ShaderWrite;
        auto swapchainImage = RHITextureUsage::RenderTarget | RHITextureUsage::Present;

        EXPECT_TRUE(HasFlag(colorAttachment, RHITextureUsage::RenderTarget));
        EXPECT_TRUE(HasFlag(depthAttachment, RHITextureUsage::DepthStencil));
        EXPECT_TRUE(HasFlag(storageImage, RHITextureUsage::ShaderWrite));
        EXPECT_TRUE(HasFlag(swapchainImage, RHITextureUsage::Present));
    }
}
