// Unit tests for RHIImageLayout enum.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHIImageLayout) == 4,
                  "RHIImageLayout must be 4 bytes (u32)");

    TEST(RHIImageLayout, ValuesAreContiguous)
    {
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::Undefined), 0u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::General), 1u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::ColorAttachmentOptimal), 2u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::DepthStencilAttachmentOptimal), 3u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::DepthStencilReadOnlyOptimal), 4u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::ShaderReadOnlyOptimal), 5u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::TransferSrcOptimal), 6u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::TransferDstOptimal), 7u);
        EXPECT_EQ(static_cast<u32>(RHIImageLayout::PresentSrc), 8u);
    }

    TEST(RHIImageLayout, IsAssignable)
    {
        RHIImageLayout layout = RHIImageLayout::ColorAttachmentOptimal;
        EXPECT_EQ(layout, RHIImageLayout::ColorAttachmentOptimal);
        layout = RHIImageLayout::ShaderReadOnlyOptimal;
        EXPECT_EQ(layout, RHIImageLayout::ShaderReadOnlyOptimal);
    }

    TEST(RHIImageLayout, TypicalTransitionChain)
    {
        // Audit 3.7: shadow depth pass → sampled depth
        RHIImageLayout shadowDepth = RHIImageLayout::DepthStencilAttachmentOptimal;
        RHIImageLayout sampled = RHIImageLayout::ShaderReadOnlyOptimal;
        EXPECT_NE(shadowDepth, sampled);
        EXPECT_EQ(shadowDepth, RHIImageLayout::DepthStencilAttachmentOptimal);
        EXPECT_EQ(sampled, RHIImageLayout::ShaderReadOnlyOptimal);
    }
}
