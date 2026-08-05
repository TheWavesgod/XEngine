// Unit tests for RHIAccessFlags flag enum.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHIAccessFlags) == 4,
                  "RHIAccessFlags must be 4 bytes (u32)");

    TEST(RHIAccessFlags, ValuesAreBitFlags)
    {
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::None), 0u);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::IndirectCommandRead), 1u << 0);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::IndexRead), 1u << 1);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::VertexAttributeRead), 1u << 2);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::UniformRead), 1u << 3);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::InputAttachmentRead), 1u << 4);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::ShaderRead), 1u << 5);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::ShaderWrite), 1u << 6);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::ColorAttachmentRead), 1u << 7);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::ColorAttachmentWrite), 1u << 8);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::DepthStencilAttachmentRead), 1u << 9);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::DepthStencilAttachmentWrite), 1u << 10);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::TransferRead), 1u << 11);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::TransferWrite), 1u << 12);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::HostRead), 1u << 13);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::HostWrite), 1u << 14);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::MemoryRead), 1u << 15);
        EXPECT_EQ(static_cast<u32>(RHIAccessFlags::MemoryWrite), 1u << 16);
    }

    TEST(RHIAccessFlags, HasFlag)
    {
        auto access = RHIAccessFlags::ShaderRead | RHIAccessFlags::ShaderWrite;
        EXPECT_TRUE(HasFlag(access, RHIAccessFlags::ShaderRead));
        EXPECT_TRUE(HasFlag(access, RHIAccessFlags::ShaderWrite));
        EXPECT_FALSE(HasFlag(access, RHIAccessFlags::ColorAttachmentWrite));
        EXPECT_FALSE(HasFlag(access, RHIAccessFlags::None));
    }

    TEST(RHIAccessFlags, TypicalBarrierCombinations)
    {
        // Color attachment write → fragment shader read
        auto src = RHIAccessFlags::ColorAttachmentWrite;
        auto dst = RHIAccessFlags::ShaderRead;
        EXPECT_NE(src, dst);
        EXPECT_EQ(static_cast<u32>(src), 1u << 8);
        EXPECT_EQ(static_cast<u32>(dst), 1u << 5);
    }
}
