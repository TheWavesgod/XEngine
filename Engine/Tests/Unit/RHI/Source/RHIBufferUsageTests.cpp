// Unit tests for RHIBufferUsage flag enum.
//
// Verifies enum values, ABI size, and flag operations (HasFlag, operator|).
// The generic flag utilities (HasFlag, operator overloads) are tested in
// RHIFlagsTests.cpp; this file focuses on the RHIBufferUsage-specific values.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHIBufferUsage) == 4,
                  "RHIBufferUsage must be 4 bytes (u32) for ABI stability");

    TEST(RHIBufferUsage, ValuesAreContiguous)
    {
        // Enum values are part of the ABI; document them.
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::None), 0u);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::Vertex), 1u << 0);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::Index), 1u << 1);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::Uniform), 1u << 2);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::Storage), 1u << 3);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::TransferSrc), 1u << 4);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::TransferDst), 1u << 5);
        EXPECT_EQ(static_cast<u32>(RHIBufferUsage::Indirect), 1u << 6);
    }

    TEST(RHIBufferUsage, HasFlagSingleBit)
    {
        auto usage = RHIBufferUsage::Vertex;
        EXPECT_TRUE(HasFlag(usage, RHIBufferUsage::Vertex));
        EXPECT_FALSE(HasFlag(usage, RHIBufferUsage::Index));
        EXPECT_FALSE(HasFlag(usage, RHIBufferUsage::None));
    }

    TEST(RHIBufferUsage, HasFlagMultipleBits)
    {
        auto usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst;
        EXPECT_TRUE(HasFlag(usage, RHIBufferUsage::Vertex));
        EXPECT_TRUE(HasFlag(usage, RHIBufferUsage::TransferDst));
        EXPECT_FALSE(HasFlag(usage, RHIBufferUsage::Index));
        EXPECT_FALSE(HasFlag(usage, RHIBufferUsage::Uniform));
    }

    TEST(RHIBufferUsage, OperatorOrCombinesFlags)
    {
        auto combined = RHIBufferUsage::Vertex | RHIBufferUsage::Index;
        EXPECT_TRUE(HasFlag(combined, RHIBufferUsage::Vertex));
        EXPECT_TRUE(HasFlag(combined, RHIBufferUsage::Index));
    }

    TEST(RHIBufferUsage, OperatorAndIntersectsFlags)
    {
        auto combined = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst;
        auto shared = combined & RHIBufferUsage::Vertex;
        EXPECT_TRUE(HasFlag(shared, RHIBufferUsage::Vertex));
        EXPECT_FALSE(HasFlag(shared, RHIBufferUsage::TransferDst));
    }

    TEST(RHIBufferUsage, TypicalBackendUsage)
    {
        // Common combinations seen in real rendering loops.
        auto vertexBuffer = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst;
        auto indexBuffer  = RHIBufferUsage::Index  | RHIBufferUsage::TransferDst;
        auto uniformBuffer = RHIBufferUsage::Uniform;  // typically HOST_VISIBLE
        auto storageBuffer = RHIBufferUsage::Storage | RHIBufferUsage::TransferDst;
        auto stagingBuffer = RHIBufferUsage::TransferSrc;

        EXPECT_TRUE(HasFlag(vertexBuffer, RHIBufferUsage::Vertex));
        EXPECT_TRUE(HasFlag(indexBuffer, RHIBufferUsage::Index));
        EXPECT_TRUE(HasFlag(uniformBuffer, RHIBufferUsage::Uniform));
        EXPECT_TRUE(HasFlag(storageBuffer, RHIBufferUsage::Storage));
        EXPECT_TRUE(HasFlag(stagingBuffer, RHIBufferUsage::TransferSrc));
    }
}
