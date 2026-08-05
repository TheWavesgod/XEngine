// Unit tests for RHIBuffer abstract interface.
//
// M4 surface: GetSize / GetUsage / Update / Map / Unmap. Tests verify the
// virtual dispatch contract via a stub device that creates in-memory buffers.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

#include <memory>
#include <vector>

namespace XEngine
{
    // Minimal RHIInstance stub for buffer tests.
    class BufferTestInstance : public RHIInstance
    {
    public:
        BufferTestInstance()
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

    // Minimal StubBuffer — records size/usage from desc.
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
        u32 GetMapCount() const noexcept { return m_MapCount; }
        u32 GetUnmapCount() const noexcept { return m_UnmapCount; }

    private:
        RHIBufferDesc m_Desc;
        u32 m_UpdateCount = 0;
        u32 m_MapCount = 0;
        u32 m_UnmapCount = 0;
    };

    // BufferTestDevice — implements CreateBufferImpl + M3 surface.
    class BufferTestDevice : public RHIDevice
    {
    public:
        explicit BufferTestDevice(RHIInstance& owner)
            : RHIDevice(owner, RHIBackend::Vulkan)
        {
        }

        void WaitIdle() override {}
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIQueue* GetQueue(RHIQueueType) const override { return nullptr; }

        RHIBuffer* CreateBufferImpl(const RHIBufferDesc& desc) override
        {
            m_LastBuffer = std::make_unique<StubBuffer>(*this, desc);
            return m_LastBuffer.get();
        }

        RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc&) override { return nullptr; }
        RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        StubBuffer* GetLastBuffer() noexcept { return m_LastBuffer.get(); }

    private:
        RHICapabilities m_Caps;
        std::unique_ptr<StubBuffer> m_LastBuffer;
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHIBuffer>,
                  "RHIBuffer must be polymorphic (virtual dtor + virtual methods)");

    TEST(RHIBuffer, GetSizeAndUsageReflectDesc)
    {
        BufferTestInstance instance;
        BufferTestDevice device(instance);

        RHIBufferDesc desc{
            .Size = 1024,
            .Usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst,
        };
        RHIBuffer* buffer = device.CreateBuffer(desc);
        ASSERT_NE(buffer, nullptr);

        EXPECT_EQ(buffer->GetSize(), 1024u);
        EXPECT_EQ(buffer->GetUsage(), RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst);
    }

    TEST(RHIBuffer, OwnerDeviceIsTheDeviceItCameFrom)
    {
        BufferTestInstance instance;
        BufferTestDevice device(instance);

        RHIBuffer* buffer = device.CreateBuffer({
            .Size = 256,
            .Usage = RHIBufferUsage::Uniform,
        });
        ASSERT_NE(buffer, nullptr);

        EXPECT_EQ(buffer->GetOwnerDevice(), &device);
        EXPECT_EQ(buffer->GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIBuffer, UpdateIsCallable)
    {
        BufferTestInstance instance;
        BufferTestDevice device(instance);

        RHIBuffer* buffer = device.CreateBuffer({
            .Size = 64,
            .Usage = RHIBufferUsage::Uniform,
        });
        ASSERT_NE(buffer, nullptr);

        StubBuffer* stub = device.GetLastBuffer();
        ASSERT_NE(stub, nullptr);

        const u8 data[] = {0xAA, 0xBB, 0xCC, 0xDD};
        buffer->Update(0, data, sizeof(data));
        buffer->Update(4, data, sizeof(data));
        EXPECT_EQ(stub->GetUpdateCount(), 2u);
    }

    TEST(RHIBuffer, MapReturnsNullptrInStub)
    {
        // Stub returns nullptr (Device-local would yield nullptr in real backend).
        BufferTestInstance instance;
        BufferTestDevice device(instance);

        RHIBuffer* buffer = device.CreateBuffer({
            .Size = 64,
            .Usage = RHIBufferUsage::Storage,
        });
        ASSERT_NE(buffer, nullptr);

        StubBuffer* stub = device.GetLastBuffer();
        void* mapped = buffer->Map();
        EXPECT_EQ(mapped, nullptr);
        EXPECT_EQ(stub->GetMapCount(), 1u);

        buffer->Unmap();
        EXPECT_EQ(stub->GetUnmapCount(), 1u);
    }

    TEST(RHIBuffer, IsPolymorphicThroughBasePointer)
    {
        BufferTestInstance instance;
        BufferTestDevice device(instance);

        RHIBuffer* buffer = device.CreateBuffer({
            .Size = 1024,
            .Usage = RHIBufferUsage::Storage | RHIBufferUsage::TransferDst,
        });
        ASSERT_NE(buffer, nullptr);

        RHIBuffer* base = buffer;
        EXPECT_EQ(base->GetSize(), 1024u);
        EXPECT_EQ(base->GetUsage(), RHIBufferUsage::Storage | RHIBufferUsage::TransferDst);
    }
}
