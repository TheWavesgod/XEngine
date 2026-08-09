// Unit tests for RHIFence abstract interface.
//
// M6 surface: IsSignaled + Wait. Verifies virtual dispatch via stubs.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

#include "RHITestStubs.h"

namespace XEngine
{
    // StatefulFence — derives from the shared Test::StubFence and adds
    // signaled state + wait-count tracking for these tests.
    class StatefulFence final : public Test::StubFence
    {
    public:
        explicit StatefulFence(RHIDevice& owner)
            : Test::StubFence(owner)
        {
        }

        bool IsSignaled() const noexcept override { return m_Signaled; }

        bool Wait(u64 timeoutNanoseconds = UINT64_MAX) noexcept override
        {
            (void)timeoutNanoseconds;
            ++m_WaitCount;
            m_Signaled = true;  // stub: wait always succeeds
            return true;
        }

        void SetSignaled(bool s) noexcept { m_Signaled = s; }
        u32  GetWaitCount() const noexcept { return m_WaitCount; }

    private:
        bool m_Signaled = false;
        u32  m_WaitCount = 0;
    };
}

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;
    using StubDevice   = Test::StubDevice;

    static_assert(std::is_polymorphic_v<RHIFence>, "RHIFence must be polymorphic");

    TEST(RHIFence, InitialStateIsUnsignaled)
    {
        StubInstance instance;
        StubDevice device(instance);
        StatefulFence fence(device);
        EXPECT_FALSE(fence.IsSignaled());
    }

    TEST(RHIFence, SetSignaledUpdatesIsSignaled)
    {
        StubInstance instance;
        StubDevice device(instance);
        StatefulFence fence(device);

        fence.SetSignaled(true);
        EXPECT_TRUE(fence.IsSignaled());
    }

    TEST(RHIFence, WaitInvokesStubAndSetsSignaled)
    {
        StubInstance instance;
        StubDevice device(instance);
        StatefulFence fence(device);

        EXPECT_EQ(fence.GetWaitCount(), 0u);
        bool result = fence.Wait();
        EXPECT_TRUE(result);
        EXPECT_EQ(fence.GetWaitCount(), 1u);
        EXPECT_TRUE(fence.IsSignaled());
    }

    TEST(RHIFence, IsPolymorphicThroughBasePointer)
    {
        StubInstance instance;
        StubDevice device(instance);
        StatefulFence fence(device);

        RHIFence* base = &fence;
        // Polymorphic dispatch: base pointer drives stub waiter.
        bool result = base->Wait();
        EXPECT_TRUE(result);
        EXPECT_TRUE(base->IsSignaled());
    }
}
