// Unit tests for RHIPipelineStage flag enum.
//
// Verifies enum values, ABI size, and flag operations.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHIPipelineStage) == 4,
                  "RHIPipelineStage must be 4 bytes (u32) for ABI stability");

    TEST(RHIPipelineStage, ValuesAreBitFlags)
    {
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::None), 0u);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::TopOfPipe), 1u << 0);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::DrawIndirect), 1u << 1);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::VertexInput), 1u << 2);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::VertexShader), 1u << 3);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::FragmentShader), 1u << 4);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::EarlyFragmentTests), 1u << 5);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::LateFragmentTests), 1u << 6);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::ColorAttachmentOutput), 1u << 7);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::ComputeShader), 1u << 8);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::Transfer), 1u << 9);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::BottomOfPipe), 1u << 10);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::Host), 1u << 11);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::AllGraphics), 1u << 12);
        EXPECT_EQ(static_cast<u32>(RHIPipelineStage::AllCommands), 1u << 13);
    }

    TEST(RHIPipelineStage, HasFlag)
    {
        auto stage = RHIPipelineStage::VertexShader | RHIPipelineStage::FragmentShader;
        EXPECT_TRUE(HasFlag(stage, RHIPipelineStage::VertexShader));
        EXPECT_TRUE(HasFlag(stage, RHIPipelineStage::FragmentShader));
        EXPECT_FALSE(HasFlag(stage, RHIPipelineStage::ComputeShader));
        EXPECT_FALSE(HasFlag(stage, RHIPipelineStage::None));
    }

    TEST(RHIPipelineStage, TypicalBarrierCombinations)
    {
        // Color attachment write → shader read for post-process
        auto src = RHIPipelineStage::ColorAttachmentOutput;
        auto dst = RHIPipelineStage::FragmentShader;
        EXPECT_NE(src, dst);
        EXPECT_EQ(static_cast<u32>(src), 1u << 7);
        EXPECT_EQ(static_cast<u32>(dst), 1u << 4);
    }
}
