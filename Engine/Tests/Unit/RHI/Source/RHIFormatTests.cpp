// Unit tests for RHIFormat enum.
//
// Verifies enum values, ABI size, and that the format set covers what
// M5 needs (8-bit color, 16/32-bit float, depth, compressed).

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>

namespace
{
    using namespace XEngine;

    static_assert(sizeof(RHIFormat) == 4,
                  "RHIFormat must be 4 bytes (u32) for ABI stability");

    TEST(RHIFormat, UnknownSentinelIsZero)
    {
        EXPECT_EQ(static_cast<u32>(RHIFormat::Unknown), 0u);
    }

    TEST(RHIFormat, ValuesAreContiguous)
    {
        // Enum values are part of the ABI; document them.
        // Test that they're contiguous starting from 1.
        u32 expected = 1;
        auto check = [&](RHIFormat fmt) {
            EXPECT_EQ(static_cast<u32>(fmt), expected) << "format value drift";
            ++expected;
        };
        check(RHIFormat::R8_UNORM);
        check(RHIFormat::R8G8_UNORM);
        check(RHIFormat::R8G8B8A8_UNORM);
        check(RHIFormat::R8G8B8A8_SRGB);
        check(RHIFormat::R16_FLOAT);
        check(RHIFormat::R16G16B16A16_FLOAT);
        check(RHIFormat::R32_FLOAT);
        check(RHIFormat::R32G32B32A32_FLOAT);
        check(RHIFormat::D32_FLOAT);
        check(RHIFormat::D24_UNORM_S8_UINT);
        check(RHIFormat::BC1_RGB_UNORM);
        check(RHIFormat::BC3_UNORM);
        check(RHIFormat::BC5_UNORM);
        check(RHIFormat::BC7_UNORM);
    }

    TEST(RHIFormat, CoversCategories)
    {
        // 8-bit color
        EXPECT_NE(static_cast<u32>(RHIFormat::R8_UNORM), 0u);
        EXPECT_NE(static_cast<u32>(RHIFormat::R8G8B8A8_UNORM), 0u);
        EXPECT_NE(static_cast<u32>(RHIFormat::R8G8B8A8_SRGB), 0u);

        // Float
        EXPECT_NE(static_cast<u32>(RHIFormat::R16_FLOAT), 0u);
        EXPECT_NE(static_cast<u32>(RHIFormat::R32_FLOAT), 0u);

        // Depth
        EXPECT_NE(static_cast<u32>(RHIFormat::D32_FLOAT), 0u);
        EXPECT_NE(static_cast<u32>(RHIFormat::D24_UNORM_S8_UINT), 0u);

        // Compressed
        EXPECT_NE(static_cast<u32>(RHIFormat::BC1_RGB_UNORM), 0u);
        EXPECT_NE(static_cast<u32>(RHIFormat::BC7_UNORM), 0u);
    }

    TEST(RHIFormat, IsAssignable)
    {
        RHIFormat fmt = RHIFormat::R8G8B8A8_SRGB;
        EXPECT_EQ(fmt, RHIFormat::R8G8B8A8_SRGB);
        fmt = RHIFormat::D32_FLOAT;
        EXPECT_EQ(fmt, RHIFormat::D32_FLOAT);
    }
}
