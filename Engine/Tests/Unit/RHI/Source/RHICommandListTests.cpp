// Unit tests for RHICommandList abstract interface.
//
// M6 surface: Begin / End / TransitionTexture (audit 3.7). Tests verify
// virtual dispatch via stubs.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHISampler.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class CListTestInstance : public RHIInstance
    {
    public:
        CListTestInstance() : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan) {}
        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override { return {}; }
        RHIDevice* CreateDevice(RHIAdapter&, const RHIDeviceDesc&) override { return nullptr; }
    };

    class CListTestDevice : public RHIDevice
    {
    public:
        CListTestDevice(RHIInstance& owner) : RHIDevice(owner, RHIBackend::Vulkan) {}
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

    class StubCommandList : public RHICommandList
    {
    public:
        StubCommandList(RHIDevice& owner) : RHICommandList(owner, owner.GetBackend()) {}

        void Begin() override { m_BeginCount++; }
        void End() override { m_EndCount++; }
        void TransitionTexture(
            RHITexture* texture,
            RHIImageLayout oldLayout,
            RHIImageLayout newLayout,
            RHIAccessFlags srcAccess,
            RHIAccessFlags dstAccess) override
        {
            m_TransitionCount++;
            m_LastTexture = texture;
            m_LastOldLayout = oldLayout;
            m_LastNewLayout = newLayout;
            m_LastSrcAccess = srcAccess;
            m_LastDstAccess = dstAccess;
        }

        u32 GetBeginCount() const noexcept { return m_BeginCount; }
        u32 GetEndCount() const noexcept { return m_EndCount; }
        u32 GetTransitionCount() const noexcept { return m_TransitionCount; }
        RHITexture* GetLastTexture() const noexcept { return m_LastTexture; }
        RHIImageLayout GetLastOldLayout() const noexcept { return m_LastOldLayout; }
        RHIImageLayout GetLastNewLayout() const noexcept { return m_LastNewLayout; }
        RHIAccessFlags GetLastSrcAccess() const noexcept { return m_LastSrcAccess; }
        RHIAccessFlags GetLastDstAccess() const noexcept { return m_LastDstAccess; }

    private:
        u32 m_BeginCount = 0;
        u32 m_EndCount = 0;
        u32 m_TransitionCount = 0;
        RHITexture* m_LastTexture = nullptr;
        RHIImageLayout m_LastOldLayout = RHIImageLayout::Undefined;
        RHIImageLayout m_LastNewLayout = RHIImageLayout::Undefined;
        RHIAccessFlags m_LastSrcAccess = RHIAccessFlags::None;
        RHIAccessFlags m_LastDstAccess = RHIAccessFlags::None;
    };

    // Forward stub for testing only.
    class StubTexture : public RHITexture
    {
    public:
        StubTexture(RHIDevice& owner) : RHITexture(owner, owner.GetBackend()) {}

        RHIFormat          GetFormat()      const noexcept override { return RHIFormat::D32_FLOAT; }
        RHITextureDimension GetDimension() const noexcept override { return RHITextureDimension::Texture2D; }
        u32                GetWidth()      const noexcept override { return 1920; }
        u32                GetHeight()     const noexcept override { return 1080; }
        u32                GetDepth()      const noexcept override { return 1; }
        u32                GetMipLevels()  const noexcept override { return 1; }
        u32                GetArrayLayers() const noexcept override { return 1; }
        RHITextureUsage    GetUsage()      const noexcept override { return RHITextureUsage::DepthStencil; }
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHICommandList>,
                  "RHICommandList must be polymorphic");

    TEST(RHICommandList, BeginEndAreCallable)
    {
        CListTestInstance instance;
        CListTestDevice device(instance);
        StubCommandList cmdList(device);

        EXPECT_EQ(cmdList.GetBeginCount(), 0u);
        EXPECT_EQ(cmdList.GetEndCount(), 0u);

        cmdList.Begin();
        EXPECT_EQ(cmdList.GetBeginCount(), 1u);

        cmdList.End();
        EXPECT_EQ(cmdList.GetEndCount(), 1u);
    }

    TEST(RHICommandList, TransitionTextureRecordsArgs)
    {
        // Audit 3.7: explicit texture layout transition
        CListTestInstance instance;
        CListTestDevice device(instance);
        StubCommandList cmdList(device);
        StubTexture texture(device);

        cmdList.TransitionTexture(
            &texture,
            RHIImageLayout::DepthStencilAttachmentOptimal,
            RHIImageLayout::ShaderReadOnlyOptimal,
            RHIAccessFlags::DepthStencilAttachmentWrite,
            RHIAccessFlags::ShaderRead);

        EXPECT_EQ(cmdList.GetTransitionCount(), 1u);
        EXPECT_EQ(cmdList.GetLastTexture(), &texture);
        EXPECT_EQ(cmdList.GetLastOldLayout(), RHIImageLayout::DepthStencilAttachmentOptimal);
        EXPECT_EQ(cmdList.GetLastNewLayout(), RHIImageLayout::ShaderReadOnlyOptimal);
        EXPECT_EQ(cmdList.GetLastSrcAccess(), RHIAccessFlags::DepthStencilAttachmentWrite);
        EXPECT_EQ(cmdList.GetLastDstAccess(), RHIAccessFlags::ShaderRead);
    }

    TEST(RHICommandList, OwnerDeviceIsTheDeviceItCameFrom)
    {
        CListTestInstance instance;
        CListTestDevice device(instance);
        StubCommandList cmdList(device);

        EXPECT_EQ(cmdList.GetOwnerDevice(), &device);
        EXPECT_EQ(cmdList.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHICommandList, IsPolymorphicThroughBasePointer)
    {
        CListTestInstance instance;
        CListTestDevice device(instance);
        StubCommandList cmdList(device);

        RHICommandList* base = &cmdList;
        base->Begin();
        EXPECT_EQ(cmdList.GetBeginCount(), 1u);
    }
}
