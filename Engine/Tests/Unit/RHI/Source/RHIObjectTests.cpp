// Unit tests for RHIObject base class.
//
// The real RHIDevice lands in M3, so this test file defines a minimal local
// stub of RHIDevice purely to construct RHIObject instances. When M3 lands
// the stub will be replaced with the real RHIDevice and these tests will
// exercise the same construction through the production class.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIFlags.h>

#include <string_view>

namespace XEngine
{
    // Minimal test stub for RHIDevice — replaced in M3.
    class RHIDevice
    {
    public:
        RHIDevice() = default;
    };
}

namespace
{
    using namespace XEngine;

    // A concrete subclass of RHIObject used to exercise the base class.
    // Overrides SetDebugName to record the call so we can verify virtual dispatch.
    class TestRHIObject : public RHIObject
    {
    public:
        TestRHIObject(RHIDevice& device)
            : RHIObject(device, RHIBackend::Vulkan)
        {
        }

        bool SetDebugNameCalled = false;
        std::string_view LastName;

        void SetDebugName(std::string_view name) noexcept override
        {
            SetDebugNameCalled = true;
            LastName = name;
        }
    };

    // A second subclass that does NOT override SetDebugName — exercises the
    // default no-op virtual implementation.
    class TestRHIObjectDefault : public RHIObject
    {
    public:
        TestRHIObjectDefault(RHIDevice& device)
            : RHIObject(device)
        {
        }
    };

    // Flag-like enum used to verify RHIFlags integration with RHIObject's
    // expected usage patterns (which is the intended M1 use case).
    enum class TestFlags : std::uint32_t
    {
        None  = 0,
        Bit0  = 1u << 0,
        Bit1  = 1u << 1,
        Bit2  = 1u << 2,
    };

    // ---------------------------------------------------------------------
    static_assert(sizeof(RHIObject) >= sizeof(void*),
                  "RHIObject must carry a vtable pointer for virtual dispatch");
    static_assert(std::is_polymorphic_v<RHIObject>,
                  "RHIObject must be polymorphic (virtual dtor + SetDebugName)");
    static_assert(sizeof(RHIBackend) == 1,
                  "RHIBackend must stay 1 byte for ABI stability");

    // ---------------------------------------------------------------------
    TEST(RHIObject, ConstructWithOwner)
    {
        RHIDevice device;
        TestRHIObject obj(device);

        EXPECT_EQ(obj.GetOwnerDevice(), &device);
        EXPECT_EQ(obj.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIObject, ConstructWithoutBackendDefaultsToNone)
    {
        RHIDevice device;
        TestRHIObjectDefault obj(device);

        EXPECT_EQ(obj.GetOwnerDevice(), &device);
        EXPECT_EQ(obj.GetBackend(), RHIBackend::None);
    }

    TEST(RHIObject, SetDebugNameDispatchesVirtually)
    {
        RHIDevice device;
        TestRHIObject obj(device);

        EXPECT_FALSE(obj.SetDebugNameCalled);
        obj.SetDebugName("MyBuffer");
        EXPECT_TRUE(obj.SetDebugNameCalled);
        EXPECT_EQ(obj.LastName, "MyBuffer");
    }

    TEST(RHIObject, SetDebugNameDefaultIsNoOp)
    {
        RHIDevice device;
        TestRHIObjectDefault obj(device);

        // Default implementation must not crash or alter state.
        obj.SetDebugName("Anything");
        // No observable state — reaching here is the test.
        SUCCEED();
    }

    // ---------------------------------------------------------------------
    // RHIFlags integration smoke test (full coverage lives in RHIFlagsTests.cpp).
    TEST(RHIObject, FlagsWorkOnBackendTagPattern)
    {
        TestFlags f = TestFlags::Bit0;
        EXPECT_TRUE(HasFlag(f, TestFlags::Bit0));
        EXPECT_FALSE(HasFlag(f, TestFlags::Bit1));
        EXPECT_FALSE(HasFlag(f, TestFlags::None));

        TestFlags g = EnableFlag(f, TestFlags::Bit1);
        EXPECT_TRUE(HasFlag(g, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(g, TestFlags::Bit1));

        TestFlags h = DisableFlag(g, TestFlags::Bit0);
        EXPECT_FALSE(HasFlag(h, TestFlags::Bit0));
        EXPECT_TRUE(HasFlag(h, TestFlags::Bit1));
    }
}
