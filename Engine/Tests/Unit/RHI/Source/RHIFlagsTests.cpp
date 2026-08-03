// Unit tests for RHIFlags utilities.
//
// RHIFlags provides HasFlag, HasAnyFlag, EnableFlag, DisableFlag, and
// CombineFlags as templates over any enum type with underlying integer type.
// These tests exercise them on a representative test enum and assert the
// semantics at compile time (static_assert) and runtime (EXPECT_*).

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIFlags.h>

#include <type_traits>

namespace
{
    using namespace XEngine;

    // A representative flag enum mirroring the shape of RHIBufferUsage /
    // RHITextureUsage / RHIShaderStage definitions that will land in
    // later milestones.
    enum class TestFlags : std::uint32_t
    {
        None  = 0,
        Bit0  = 1u << 0,
        Bit1  = 1u << 1,
        Bit2  = 1u << 2,
        Bit3  = 1u << 3,
    };

    // ---------------------------------------------------------------------
    // Compile-time checks.
    static_assert(HasFlag(TestFlags::Bit0, TestFlags::Bit0),
                  "single bit must set its own flag");
    static_assert(!HasFlag(TestFlags::Bit0, TestFlags::Bit1),
                  "different bits must not be flagged");
    static_assert(!HasFlag(TestFlags::Bit0, TestFlags::None),
                  "None flag must test as false");
    static_assert(HasFlag(TestFlags::Bit0 | TestFlags::Bit1, TestFlags::Bit0),
                  "OR'd value must contain Bit0");
    static_assert(HasFlag(TestFlags::Bit0 | TestFlags::Bit1, TestFlags::Bit1),
                  "OR'd value must contain Bit1");

    static_assert(HasAnyFlag(TestFlags::Bit0, TestFlags::Bit0),
                  "Bit0 must be 'any-set' against itself");
    static_assert(!HasAnyFlag(TestFlags::Bit0, TestFlags::Bit1),
                  "Bit0 must NOT share bits with Bit1");
    static_assert(HasAnyFlag(TestFlags::Bit0 | TestFlags::Bit1, TestFlags::Bit1),
                  "OR'd value must contain Bit1");
    static_assert(!HasAnyFlag(TestFlags::None, TestFlags::Bit0),
                  "None must never be 'any-set' against anything");

    static_assert(EnableFlag(TestFlags::Bit0, TestFlags::Bit1) == (TestFlags::Bit0 | TestFlags::Bit1),
                  "EnableFlag must equal OR");
    static_assert(DisableFlag(TestFlags::Bit0 | TestFlags::Bit1, TestFlags::Bit0) == TestFlags::Bit1,
                  "DisableFlag must clear Bit0 only");
    static_assert(CombineFlags(TestFlags::Bit0, TestFlags::Bit1, TestFlags::Bit2)
                  == (TestFlags::Bit0 | TestFlags::Bit1 | TestFlags::Bit2),
                  "CombineFlags must OR all flags");

    // ---------------------------------------------------------------------
    TEST(RHIFlags, HasFlagSingleBit)
    {
        TestFlags f = TestFlags::Bit0;
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit1));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit2));
    }

    TEST(RHIFlags, HasFlagMultiBit)
    {
        TestFlags f = TestFlags::Bit0 | TestFlags::Bit2;
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit2));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit1));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit3));
    }

    TEST(RHIFlags, HasFlagNoneAlwaysFalse)
    {
        EXPECT_FALSE(HasFlag(TestFlags::None, TestFlags::None));
        EXPECT_FALSE(HasFlag(TestFlags::Bit0, TestFlags::None));
        EXPECT_FALSE(HasFlag(TestFlags::Bit0 | TestFlags::Bit1, TestFlags::None));
    }

    TEST(RHIFlags, HasAnyFlag)
    {
        TestFlags f = TestFlags::Bit0;
        EXPECT_TRUE(HasAnyFlag(f, TestFlags::Bit0));
        EXPECT_TRUE(HasAnyFlag(f, TestFlags::Bit0 | TestFlags::Bit1));
        EXPECT_FALSE(HasAnyFlag(f, TestFlags::Bit1));
        EXPECT_FALSE(HasAnyFlag(f, TestFlags::Bit2 | TestFlags::Bit3));
    }

    TEST(RHIFlags, EnableFlag)
    {
        TestFlags f = TestFlags::Bit0;
        f = EnableFlag(f, TestFlags::Bit1);
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit1));

        // Enabling already-set flag must be idempotent.
        f = EnableFlag(f, TestFlags::Bit0);
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
    }

    TEST(RHIFlags, DisableFlag)
    {
        TestFlags f = TestFlags::Bit0 | TestFlags::Bit1 | TestFlags::Bit2;
        f = DisableFlag(f, TestFlags::Bit1);
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit1));
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit2));

        // Disabling not-set flag must be a no-op.
        f = DisableFlag(f, TestFlags::Bit3);
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit2));
    }

    TEST(RHIFlags, CombineFlagsVariadic)
    {
        TestFlags a = TestFlags::Bit0;
        TestFlags b = TestFlags::Bit1;
        TestFlags c = TestFlags::Bit2;

        TestFlags all = CombineFlags(a, b, c);
        EXPECT_TRUE(HasFlag(all, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(all, TestFlags::Bit1));
        EXPECT_TRUE(HasFlag(all, TestFlags::Bit2));
        EXPECT_FALSE(HasFlag(all, TestFlags::Bit3));

        // Single-arg form must round-trip.
        TestFlags one = CombineFlags(TestFlags::Bit1);
        EXPECT_EQ(one, TestFlags::Bit1);
    }

    // ---------------------------------------------------------------------
    // A second flag enum with a different underlying type to confirm the
    // templates are not coupled to std::uint32_t.
    enum class SmallFlags : std::uint8_t
    {
        None = 0,
        A    = 1 << 0,
        B    = 1 << 1,
    };

    TEST(RHIFlags, WorksOnSmallerUnderlyingType)
    {
        static_assert(sizeof(SmallFlags) == 1, "SmallFlags must be 1 byte");
        SmallFlags f = SmallFlags::A | SmallFlags::B;
        EXPECT_TRUE(HasFlag(f, SmallFlags::A));
        EXPECT_TRUE(HasFlag(f, SmallFlags::B));
        EXPECT_FALSE(HasFlag(f, SmallFlags::None));
    }

    // ---------------------------------------------------------------------
    // Static type-trait guard: passing a non-enum type must fail to compile.
    // (We do not instantiate that here; the static_assert in the templates
    // enforces it at the call site.)
}
