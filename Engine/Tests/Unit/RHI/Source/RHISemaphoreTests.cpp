// Unit tests for RHISemaphore abstract interface.
//
// M6 surface: no CPU-side methods. Tests verify polymorphism + ownership.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHISampler.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class SemaphoreTestInstance : public RHIInstance
    {
    public:
        SemaphoreTestInstance() : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan) {}
        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override { return {}; }
        std::unique_ptr<RHIDevice> CreateDeviceImpl(RHIAdapter&, const RHIDeviceDesc&) override { return nullptr; }
    };

    class SemaphoreTestDevice : public RHIDevice
    {
    public:
        SemaphoreTestDevice(RHIInstance& owner) : RHIDevice(owner, RHIBackend::Vulkan) {}
        void WaitIdle() override {}
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIFeature GetEnabledFeatures() const noexcept override { return RHIFeature::None; }
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

    class StubSemaphore : public RHISemaphore
    {
    public:
        StubSemaphore(RHIDevice& owner) : RHISemaphore(owner, owner.GetBackend()) {}
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHISemaphore>, "RHISemaphore must be polymorphic");

    TEST(RHISemaphore, Construction)
    {
        SemaphoreTestInstance instance;
        SemaphoreTestDevice device(instance);
        StubSemaphore sem(device);

        EXPECT_EQ(sem.GetBackend(), RHIBackend::Vulkan);
        EXPECT_EQ(sem.GetOwnerDevice(), &device);
    }

    TEST(RHISemaphore, IsPolymorphic)
    {
        SemaphoreTestInstance instance;
        SemaphoreTestDevice device(instance);
        StubSemaphore sem(device);

        RHISemaphore* base = &sem;
        EXPECT_EQ(base->GetOwnerDevice(), &device);
    }
}
