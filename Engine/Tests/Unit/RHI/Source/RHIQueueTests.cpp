// Unit tests for RHIQueue.
//
// M3 surface is minimal: GetType() only. Tests verify identifier,
// polymorphism, and enum value stability.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    // Minimal RHIInstance stub — implements pure virtuals with no-op returns.
    class QueueTestInstance : public RHIInstance
    {
    public:
        QueueTestInstance()
            : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan)
        {
        }

        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override
        {
            return {};
        }

        RHIDevice* CreateDevice(RHIAdapter&, const RHIDeviceDesc&) override
        {
            return nullptr;
        }
    };

    // Minimal RHIDevice stub — required to construct RHIQueue.
    class QueueTestDevice : public RHIDevice
    {
    public:
        explicit QueueTestDevice(RHIInstance& owner)
            : RHIDevice(owner)
        {
        }

        void WaitIdle() override {}
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIQueue* GetQueue(RHIQueueType) const override { return nullptr; }

    private:
        RHICapabilities m_Caps;
    };

    // Stub RHIQueue — returns the type and backend it was constructed with.
    class StubQueue : public RHIQueue
    {
    public:
        StubQueue(RHIDevice& owner, RHIBackend backend, RHIQueueType type)
            : RHIQueue(owner, backend)
            , m_Type(type)
        {
        }

        RHIQueueType GetType() const noexcept override { return m_Type; }

    private:
        RHIQueueType m_Type;
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHIQueue>,
                  "RHIQueue must be polymorphic (virtual dtor + GetType)");
    static_assert(sizeof(RHIQueueType) == 1,
                  "RHIQueueType must be 1 byte for ABI stability");

    TEST(RHIQueue, GetTypeReturnsConstructedValue)
    {
        QueueTestInstance instance;
        QueueTestDevice device(instance);

        StubQueue gfx(device, RHIBackend::Vulkan, RHIQueueType::Graphics);
        StubQueue cmp(device, RHIBackend::Vulkan, RHIQueueType::Compute);
        StubQueue xfer(device, RHIBackend::Vulkan, RHIQueueType::Transfer);

        EXPECT_EQ(gfx.GetType(), RHIQueueType::Graphics);
        EXPECT_EQ(cmp.GetType(), RHIQueueType::Compute);
        EXPECT_EQ(xfer.GetType(), RHIQueueType::Transfer);
    }

    TEST(RHIQueue, OwnerDeviceIsTheOnePassedToConstructor)
    {
        QueueTestInstance instance;
        QueueTestDevice device(instance);
        StubQueue q(device, RHIBackend::Vulkan, RHIQueueType::Graphics);

        EXPECT_EQ(q.GetOwnerDevice(), &device);
        EXPECT_EQ(q.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIQueue, IsPolymorphicThroughBasePointer)
    {
        QueueTestInstance instance;
        QueueTestDevice device(instance);
        StubQueue q(device, RHIBackend::Vulkan, RHIQueueType::Compute);

        RHIQueue* base = &q;
        EXPECT_EQ(base->GetType(), RHIQueueType::Compute);

        // Reset to graphics, verify virtual dispatch picks up the change.
        // (We can't actually reassign the type here since m_Type is const after ctor;
        //  this just demonstrates that the base pointer works correctly.)
        EXPECT_EQ(base->GetOwnerDevice(), &device);
    }

    TEST(RHIQueueType, ValuesAreContiguous)
    {
        // Enum values are part of the ABI; document them.
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Graphics), 0);
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Compute), 1);
        EXPECT_EQ(static_cast<u8>(RHIQueueType::Transfer), 2);
    }
}
