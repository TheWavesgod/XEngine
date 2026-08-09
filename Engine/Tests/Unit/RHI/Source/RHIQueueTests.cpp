// Unit tests for RHIQueue.
//
// M3: GetType() only. M6: + Submit.
//
// The test-side RHIInstance / RHIDevice / RHIQueue / RHICommandList /
// RHIFence / RHISemaphore stubs come from the shared header
// "RHITestStubs.h". Duplicating them here was the source of an MSVC ODR
// bug (linker picking a sibling-TU definition with a different layout,
// corrupting the stack when the destructor ran). See RHITestStubs.h for
// the full audit trail.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHICommandList.h>

#include "RHITestStubs.h"

#include <span>

namespace
{
    using namespace XEngine;
    using StubInstance   = Test::StubInstance;
    using StubDevice     = Test::StubDevice;
    using StubQueue      = Test::StubQueue;
    using StubCommandList = Test::StubCommandList;
    using StubFence      = Test::StubFence;
    using StubSemaphore  = Test::StubSemaphore;

    static_assert(std::is_polymorphic_v<RHIQueue>,
                  "RHIQueue must be polymorphic (virtual dtor + GetType)");
    static_assert(sizeof(RHIQueueType) == 1,
                  "RHIQueueType must be 1 byte for ABI stability");

    TEST(RHIQueue, GetTypeReturnsConstructedValue)
    {
        StubInstance instance;
        StubDevice device(instance);

        StubQueue gfx(device, RHIBackend::Vulkan, RHIQueueType::Graphics);
        StubQueue cmp(device, RHIBackend::Vulkan, RHIQueueType::Compute);
        StubQueue xfer(device, RHIBackend::Vulkan, RHIQueueType::Transfer);

        EXPECT_EQ(gfx.GetType(), RHIQueueType::Graphics);
        EXPECT_EQ(cmp.GetType(), RHIQueueType::Compute);
        EXPECT_EQ(xfer.GetType(), RHIQueueType::Transfer);
    }

    TEST(RHIQueue, OwnerDeviceIsTheOnePassedToConstructor)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubQueue q(device, RHIBackend::Vulkan, RHIQueueType::Graphics);

        EXPECT_EQ(q.GetOwnerDevice(), &device);
        EXPECT_EQ(q.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIQueue, IsPolymorphicThroughBasePointer)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubQueue q(device, RHIBackend::Vulkan, RHIQueueType::Compute);

        RHIQueue* base = &q;
        EXPECT_EQ(base->GetType(), RHIQueueType::Compute);
        EXPECT_EQ(base->GetOwnerDevice(), &device);
    }

    TEST(RHIQueue, SubmitWithOnlyCmdList)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubQueue queue(device, RHIBackend::Vulkan, RHIQueueType::Graphics);

        StubCommandList cmdList(device);

        queue.Submit(&cmdList);

        EXPECT_EQ(queue.GetLastCmdList(), &cmdList);
        EXPECT_EQ(queue.GetLastSignalFence(), nullptr);
        EXPECT_TRUE(queue.GetLastWaitSemaphores().empty());
        EXPECT_TRUE(queue.GetLastSignalSemaphores().empty());
    }

    TEST(RHIQueue, SubmitWithFenceOnly)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubQueue queue(device, RHIBackend::Vulkan, RHIQueueType::Graphics);

        StubCommandList cmdList(device);
        StubFence fence(device);

        queue.Submit(&cmdList, &fence);

        EXPECT_EQ(queue.GetLastCmdList(), &cmdList);
        EXPECT_EQ(queue.GetLastSignalFence(), &fence);
        EXPECT_TRUE(queue.GetLastWaitSemaphores().empty());
        EXPECT_TRUE(queue.GetLastSignalSemaphores().empty());
    }

    TEST(RHIQueue, SubmitWithAllArgsRecordsThem)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubQueue queue(device, RHIBackend::Vulkan, RHIQueueType::Graphics);

        StubCommandList cmdList(device);
        StubFence fence(device);
        StubSemaphore waitSem1(device);
        StubSemaphore waitSem2(device);
        StubSemaphore signalSem(device);

        RHISemaphore* waitSems[]  = { &waitSem1, &waitSem2 };
        RHISemaphore* signalSems[] = { &signalSem };
        queue.Submit(&cmdList, &fence,
                      std::span<RHISemaphore*>(waitSems, 2),
                      std::span<RHISemaphore*>(signalSems, 1));

        EXPECT_EQ(queue.GetLastCmdList(), &cmdList);
        EXPECT_EQ(queue.GetLastSignalFence(), &fence);

        const auto& waits = queue.GetLastWaitSemaphores();
        EXPECT_EQ(waits.size(), 2u);
        EXPECT_EQ(waits[0], &waitSem1);
        EXPECT_EQ(waits[1], &waitSem2);

        const auto& signals = queue.GetLastSignalSemaphores();
        EXPECT_EQ(signals.size(), 1u);
        EXPECT_EQ(signals[0], &signalSem);
    }

    TEST(RHIQueueType, ValuesAreContiguous)
    {
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Graphics), 0);
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Compute), 1);
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Transfer), 2);
    }
}
