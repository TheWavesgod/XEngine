// Unit tests for RHIDevice M3 interface.
//
// M3 surface: lifecycle + queue + capabilities. Tests verify the contract
// using a stub device that exposes settable capabilities and a single
// queue of the test's choosing.
//
// M4+ will add CreateBuffer / CreateTexture tests to this file.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    // Minimal RHIInstance stub for device tests.
    class DeviceTestInstance : public RHIInstance
    {
    public:
        DeviceTestInstance()
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

    // Stub RHIQueue — used to confirm device returns the right queue.
    class StubQueue : public RHIQueue
    {
    public:
        StubQueue(RHIDevice& owner, RHIQueueType type)
            : RHIQueue(owner)
            , m_Type(type)
        {
        }

        RHIQueueType GetType() const noexcept override { return m_Type; }

    private:
        RHIQueueType m_Type;
    };

    // Stub RHIDevice — exposes settable caps / max frames / queue type.
    class StubDevice : public RHIDevice
    {
    public:
        explicit StubDevice(RHIInstance& owner, RHIQueueType qType = RHIQueueType::Graphics)
            : RHIDevice(owner)
            , m_QueueType(qType)
            , m_Queue(std::make_unique<StubQueue>(*this, qType))
        {
        }

        void WaitIdle() override { m_WaitIdleCount++; }
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return m_MaxFramesInFlight; }
        RHIQueue* GetQueue(RHIQueueType type) const override
        {
            return (type == m_QueueType) ? m_Queue.get() : nullptr;
        }

        // Test harness
        void SetMaxFramesInFlight(u32 v) noexcept { m_MaxFramesInFlight = v; }
        void SetCapabilities(RHICapabilities caps) noexcept { m_Caps = caps; }
        u32 GetWaitIdleCount() const noexcept { return m_WaitIdleCount; }
        RHIQueueType GetQueuedType() const noexcept { return m_QueueType; }

    private:
        RHIQueueType m_QueueType;
        u32 m_MaxFramesInFlight = 2;
        u32 m_WaitIdleCount = 0;
        RHICapabilities m_Caps;
        std::unique_ptr<StubQueue> m_Queue;
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHIDevice>,
                  "RHIDevice must be polymorphic (virtual dtor + virtual methods)");

    // ---------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------

    TEST(RHIDevice, WaitIdleIsCallable)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        EXPECT_EQ(device.GetWaitIdleCount(), 0u);
        device.WaitIdle();
        EXPECT_EQ(device.GetWaitIdleCount(), 1u);
        device.WaitIdle();
        device.WaitIdle();
        EXPECT_EQ(device.GetWaitIdleCount(), 3u);
    }

    // ---------------------------------------------------------------------
    // Device info
    // ---------------------------------------------------------------------

    TEST(RHIDevice, GetBackendReturnsBackendTag)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        EXPECT_EQ(device.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIDevice, GetCapabilitiesReturnsStorageReference)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        // GetCapabilities returns const& — modifications via the reference
        // are visible to the device (it's the device's internal storage).
        const RHICapabilities& caps = device.GetCapabilities();
        EXPECT_EQ(&caps, &device.GetCapabilities());  // same instance

        // Mutating caps via the reference is the backend's job (Initialize).
        device.SetCapabilities(RHICapabilities{
            .MaxTextureSize2D = 4096,
            .MaxFramesInFlight = 3,
            .SupportsTimelineSemaphore = true,
        });

        EXPECT_EQ(caps.MaxTextureSize2D, 4096u);
        EXPECT_EQ(caps.MaxFramesInFlight, 3u);
        EXPECT_TRUE(caps.SupportsTimelineSemaphore);
    }

    TEST(RHIDevice, GetMaxFramesInFlightReturnsBackendValue)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        EXPECT_EQ(device.GetMaxFramesInFlight(), 2u);

        device.SetMaxFramesInFlight(3);
        EXPECT_EQ(device.GetMaxFramesInFlight(), 3u);

        device.SetMaxFramesInFlight(1);
        EXPECT_EQ(device.GetMaxFramesInFlight(), 1u);
    }

    // ---------------------------------------------------------------------
    // Queue
    // ---------------------------------------------------------------------

    TEST(RHIDevice, GetQueueReturnsQueueForMatchingType)
    {
        DeviceTestInstance instance;
        StubDevice device(instance, RHIQueueType::Compute);

        RHIQueue* q = device.GetQueue(RHIQueueType::Compute);
        ASSERT_NE(q, nullptr);
        EXPECT_EQ(q->GetType(), RHIQueueType::Compute);
        EXPECT_EQ(q->GetOwnerDevice(), &device);
    }

    TEST(RHIDevice, GetQueueReturnsNullForNonMatchingType)
    {
        DeviceTestInstance instance;
        StubDevice device(instance, RHIQueueType::Graphics);

        EXPECT_EQ(device.GetQueue(RHIQueueType::Compute), nullptr);
        EXPECT_EQ(device.GetQueue(RHIQueueType::Transfer), nullptr);
        EXPECT_NE(device.GetQueue(RHIQueueType::Graphics), nullptr);
    }

    TEST(RHIDevice, GetQueueEachTypeReturnsItsOwn)
    {
        // A more elaborate stub could expose 3 queues; the M3 minimum
        // is "one queue per type lookup". Verifying the simple case here.
        DeviceTestInstance instance;
        StubDevice device(instance, RHIQueueType::Graphics);

        RHIQueue* q = device.GetQueue(RHIQueueType::Graphics);
        ASSERT_NE(q, nullptr);
        EXPECT_EQ(q->GetType(), RHIQueueType::Graphics);
    }
}
