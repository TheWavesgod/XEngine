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

#include "RHITestStubs.h"

#include <memory>
#include <vector>

namespace XEngine
{
    // Minimal StubBuffer — records size/usage from desc. Local because it
    // owns test-specific counters.
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

    // BufferTestDevice — derives from the shared Test::StubDevice and only
    // overrides the buffer factory hook.
    class BufferTestDevice final : public Test::StubDevice
    {
    public:
        explicit BufferTestDevice(RHIInstance& owner)
            : Test::StubDevice(owner)
        {
        }

        RHIBuffer* CreateBufferImpl(const RHIBufferDesc& desc) override
        {
            m_LastBuffer = std::make_unique<StubBuffer>(*this, desc);
            return m_LastBuffer.get();
        }

        StubBuffer* GetLastBuffer() noexcept { return m_LastBuffer.get(); }

    private:
        std::unique_ptr<StubBuffer> m_LastBuffer;
    };
}

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;

    static_assert(std::is_polymorphic_v<RHIBuffer>,
                  "RHIBuffer must be polymorphic (virtual dtor + virtual methods)");

    TEST(RHIBuffer, GetSizeAndUsageReflectDesc)
    {
        StubInstance instance;
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
        StubInstance instance;
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
        StubInstance instance;
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
        StubInstance instance;
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
        StubInstance instance;
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
