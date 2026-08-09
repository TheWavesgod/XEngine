// Unit tests for RHISemaphore abstract interface.
//
// M6 surface: no CPU-side methods. Tests verify polymorphism + ownership.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIEnums.h>

#include "RHITestStubs.h"

namespace
{
    using namespace XEngine;
    using StubInstance   = Test::StubInstance;
    using StubDevice     = Test::StubDevice;
    using StubSemaphore  = Test::StubSemaphore;

    static_assert(std::is_polymorphic_v<RHISemaphore>, "RHISemaphore must be polymorphic");

    TEST(RHISemaphore, Construction)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubSemaphore sem(device);

        EXPECT_EQ(sem.GetBackend(), RHIBackend::Vulkan);
        EXPECT_EQ(sem.GetOwnerDevice(), &device);
    }

    TEST(RHISemaphore, IsPolymorphic)
    {
        StubInstance instance;
        StubDevice device(instance);
        StubSemaphore sem(device);

        RHISemaphore* base = &sem;
        EXPECT_EQ(base->GetOwnerDevice(), &device);
    }
}
