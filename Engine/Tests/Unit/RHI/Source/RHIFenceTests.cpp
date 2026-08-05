// Unit tests for RHIFence abstract interface.
//
// M6 surface: IsSignaled + Wait. Verifies virtual dispatch via stubs.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHISampler.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class FenceTestInstance : public RHIInstance
    {
    public:
        FenceTestInstance() : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan) {}
        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override { return {}; }
        RHIDevice* CreateDevice(RHIAdapter&, const RHIDeviceDesc&) override { return nullptr; }
    };

    class FenceTestDevice : public RHIDevice
    {
    public:
        FenceTestDevice(RHIInstance& owner) : RHIDevice(owner, RHIBackend::Vulkan) {}
        void WaitIdle() override {}
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIQueue* GetQueue(RHIQueueType) const override { return nullptr; }
        RHIBuffer* CreateBufferImpl(const RHIBufferDesc&) override { return nullptr; }
        RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc&) override { return nullptr; }
        RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }
    private:
        RHICapabilities m_Caps;
    };

    class StubFence : public RHIFence
    {
    public:
        StubFence(RHIDevice& owner) : RHIFence(owner, owner.GetBackend()) {}
        bool IsSignaled() const noexcept override { return m_Signaled; }
        bool Wait(u64 timeoutNanoseconds = UINT64_MAX) noexcept override
        {
            (void)timeoutNanoseconds;
            ++m_WaitCount;
            m_Signaled = true;  // stub: wait always succeeds
            return true;
        }
        void SetSignaled(bool s) noexcept { m_Signaled = s; }
        u32 GetWaitCount() const noexcept { return m_WaitCount; }
    private:
        bool m_Signaled = false;
        u32 m_WaitCount = 0;
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHIFence>, "RHIFence must be polymorphic");

    TEST(RHIFence, InitialStateIsUnsignaled)
    {
        FenceTestInstance instance;
        FenceTestDevice device(instance);
        StubFence fence(device);
        EXPECT_FALSE(fence.IsSignaled());
    }

    TEST(RHIFence, SetSignaledUpdatesIsSignaled)
    {
        FenceTestInstance instance;
        FenceTestDevice device(instance);
        StubFence fence(device);

        fence.SetSignaled(true);
        EXPECT_TRUE(fence.IsSignaled());
    }

    TEST(RHIFence, WaitInvokesStubAndSetsSignaled)
    {
        FenceTestInstance instance;
        FenceTestDevice device(instance);
        StubFence fence(device);

        EXPECT_EQ(fence.GetWaitCount(), 0u);
        bool result = fence.Wait();
        EXPECT_TRUE(result);
        EXPECT_EQ(fence.GetWaitCount(), 1u);
        EXPECT_TRUE(fence.IsSignaled());
    }

    TEST(RHIFence, IsPolymorphicThroughBasePointer)
    {
        FenceTestInstance instance;
        FenceTestDevice device(instance);
        StubFence fence(device);

        RHIFence* base = &fence;
        // Polymorphic dispatch: base pointer drives stub waiter.
        bool result = base->Wait();
        EXPECT_TRUE(result);
        EXPECT_TRUE(base->IsSignaled());
    }
}
