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
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

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

        std::unique_ptr<RHIDevice> CreateDeviceImpl(RHIAdapter&, const RHIDeviceDesc&) override
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

        void Submit(
            RHICommandList*,
            RHIFence* = nullptr,
            std::span<RHISemaphore*> = {},
            std::span<RHISemaphore*> = {}) override
        {
        }

    private:
        RHIQueueType m_Type;
    };

    // Local buffer stub — verifies M4 surface on a stub.
    class StubBuffer : public RHIBuffer
    {
    public:
        StubBuffer(RHIDevice& owner, RHIBufferDesc desc)
            : RHIBuffer(owner, owner.GetBackend())
            , m_Desc(desc)
        {
        }

        u64 GetSize() const noexcept override { return m_Desc.Size; }
        RHIBufferUsage GetUsage() const noexcept override { return m_Desc.Usage; }
        void Update(u64, const void*, u64) override { m_UpdateCount++; }
        void* Map() override { m_MapCount++; return nullptr; }
        void Unmap() override { m_UnmapCount++; }

        u32 GetUpdateCount() const noexcept { return m_UpdateCount; }

    private:
        RHIBufferDesc m_Desc;
        u32 m_UpdateCount = 0;
        u32 m_MapCount = 0;
        u32 m_UnmapCount = 0;
    };

    // M5: local texture stub. Mirrors StubBuffer's pattern.
    class StubTexture : public RHITexture
    {
    public:
        StubTexture(RHIDevice& owner, RHITextureDesc desc)
            : RHITexture(owner, owner.GetBackend())
            , m_Desc(desc)
        {
        }

        RHIFormat          GetFormat()      const noexcept override { return m_Desc.Format; }
        RHITextureDimension GetDimension() const noexcept override { return m_Desc.Dimension; }
        u32                GetWidth()      const noexcept override { return m_Desc.Width; }
        u32                GetHeight()     const noexcept override { return m_Desc.Height; }
        u32                GetDepth()      const noexcept override { return m_Desc.Depth; }
        u32                GetMipLevels()  const noexcept override { return m_Desc.MipLevels; }
        u32                GetArrayLayers() const noexcept override { return m_Desc.ArrayLayers; }
        RHITextureUsage    GetUsage()      const noexcept override { return m_Desc.Usage; }

    private:
        RHITextureDesc m_Desc;
    };

    // M5: local texture view stub.
    class StubTextureView : public RHITextureView
    {
    public:
        StubTextureView(RHIDevice& owner, RHITextureViewDesc desc)
            : RHITextureView(owner, owner.GetBackend())
            , m_Desc(desc)
        {
        }

        RHIFormat          GetFormat()          const noexcept override { return m_Desc.Format; }
        const RHITexture*  GetSource()          const noexcept override { return m_Desc.Source; }
        RHITextureDimension GetDimension()     const noexcept override { return m_Desc.Dimension; }
        u32                GetBaseMipLevel()    const noexcept override { return m_Desc.BaseMipLevel; }
        u32                GetMipLevelCount()   const noexcept override { return m_Desc.MipLevelCount; }
        u32                GetBaseArrayLayer()  const noexcept override { return m_Desc.BaseArrayLayer; }
        u32                GetArrayLayerCount() const noexcept override { return m_Desc.ArrayLayerCount; }

    private:
        RHITextureViewDesc m_Desc;
    };

    // M5: local sampler stub. Captures desc at creation.
    class StubSampler : public RHISampler
    {
    public:
        StubSampler(RHIDevice& owner, RHISamplerDesc desc)
            : RHISampler(owner, owner.GetBackend())
            , m_Desc(desc)
        {
        }

        RHISamplerDesc GetDesc() const noexcept override { return m_Desc; }

    private:
        RHISamplerDesc m_Desc;
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
        RHIFeature GetEnabledFeatures() const noexcept override { return m_EnabledFeatures; }
        RHIQueue* GetQueue(RHIQueueType type) const override
        {
            return (type == m_QueueType) ? m_Queue.get() : nullptr;
        }

        RHIBuffer* CreateBufferImpl(const RHIBufferDesc& desc) override
        {
            m_LastBuffer = std::make_unique<StubBuffer>(*this, desc);
            return m_LastBuffer.get();
        }

        RHITexture* CreateTextureImpl(const RHITextureDesc& desc) override
        {
            m_LastTexture = std::make_unique<StubTexture>(*this, desc);
            return m_LastTexture.get();
        }

        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc& desc) override
        {
            m_LastTextureView = std::make_unique<StubTextureView>(*this, desc);
            return m_LastTextureView.get();
        }

        RHISampler* CreateSamplerImpl(const RHISamplerDesc& desc) override
        {
            m_LastSampler = std::make_unique<StubSampler>(*this, desc);
            return m_LastSampler.get();
        }

        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        // Test harness
        void SetMaxFramesInFlight(u32 v) noexcept { m_MaxFramesInFlight = v; }
        void SetCapabilities(RHICapabilities caps) noexcept { m_Caps = caps; }
        u32 GetWaitIdleCount() const noexcept { return m_WaitIdleCount; }
        RHIQueueType GetQueuedType() const noexcept { return m_QueueType; }
        StubBuffer* GetLastBuffer() noexcept { return m_LastBuffer.get(); }
        StubTexture* GetLastTexture() noexcept { return m_LastTexture.get(); }
        StubTextureView* GetLastTextureView() noexcept { return m_LastTextureView.get(); }
        StubSampler* GetLastSampler() noexcept { return m_LastSampler.get(); }

    private:
        RHIQueueType m_QueueType;
        u32 m_MaxFramesInFlight = 2;
        u32 m_WaitIdleCount = 0;
        RHICapabilities m_Caps;
        RHIFeature m_EnabledFeatures = RHIFeature::None;
        std::unique_ptr<StubQueue> m_Queue;
        std::unique_ptr<StubBuffer> m_LastBuffer;
        std::unique_ptr<StubTexture> m_LastTexture;
        std::unique_ptr<StubTextureView> m_LastTextureView;
        std::unique_ptr<StubSampler> m_LastSampler;
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

    // ---------------------------------------------------------------------
    // Resource creation (M4)
    // ---------------------------------------------------------------------

    TEST(RHIDevice, CreateBufferReturnsStubBuffer)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        RHIBufferDesc desc{
            .Size = 1024,
            .Usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst,
        };
        RHIBuffer* buffer = device.CreateBuffer(desc);
        ASSERT_NE(buffer, nullptr);

        EXPECT_EQ(buffer->GetSize(), 1024u);
        EXPECT_EQ(buffer->GetUsage(), RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst);
        EXPECT_EQ(buffer->GetOwnerDevice(), &device);

        StubBuffer* stub = device.GetLastBuffer();
        ASSERT_NE(stub, nullptr);
        EXPECT_EQ(stub->GetSize(), 1024u);
        EXPECT_EQ(stub->GetUsage(), RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst);
    }

    TEST(RHIDevice, CreateBufferSizeZeroIsNull)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        RHIBufferDesc desc{
            .Size = 0,
            .Usage = RHIBufferUsage::Vertex,
        };
        RHIBuffer* buffer = device.CreateBuffer(desc);
        EXPECT_EQ(buffer, nullptr);
        EXPECT_EQ(device.GetLastBuffer(), nullptr);
    }

    TEST(RHIDevice, CreateBufferUsageNoneIsNull)
    {
        DeviceTestInstance instance;
        StubDevice device(instance);

        RHIBufferDesc desc{
            .Size = 1024,
            .Usage = RHIBufferUsage::None,
        };
        RHIBuffer* buffer = device.CreateBuffer(desc);
        EXPECT_EQ(buffer, nullptr);
        EXPECT_EQ(device.GetLastBuffer(), nullptr);
    }
}
