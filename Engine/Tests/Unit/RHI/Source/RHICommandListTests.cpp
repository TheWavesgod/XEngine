// Unit tests for RHICommandList abstract interface.
//
// M6 surface: Begin / End / TransitionTexture (audit 3.7). Tests verify
// virtual dispatch via stubs.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHIEnums.h>

#include "RHITestStubs.h"

namespace XEngine { namespace
{
    // Local specialization: count Begin/End/TransitionTexture calls and
    // capture the last TransitionTexture arguments. Inherits the layout-
    // neutral body from Test::StubCommandList.
    class RecordingCommandList final : public Test::StubCommandList
    {
    public:
        explicit RecordingCommandList(RHIDevice& owner)
            : Test::StubCommandList(owner)
        {
        }

        void Begin() override { ++m_BeginCount; }
        void End()   override { ++m_EndCount; }

        void TransitionTexture(
            RHITexture* texture,
            RHIImageLayout oldLayout,
            RHIImageLayout newLayout,
            RHIAccessFlags srcAccess,
            RHIAccessFlags dstAccess) override
        {
            ++m_TransitionCount;
            m_LastTexture    = texture;
            m_LastOldLayout  = oldLayout;
            m_LastNewLayout  = newLayout;
            m_LastSrcAccess  = srcAccess;
            m_LastDstAccess  = dstAccess;
        }

        u32              GetBeginCount()     const noexcept { return m_BeginCount; }
        u32              GetEndCount()       const noexcept { return m_EndCount; }
        u32              GetTransitionCount() const noexcept { return m_TransitionCount; }
        RHITexture*      GetLastTexture()    const noexcept { return m_LastTexture; }
        RHIImageLayout   GetLastOldLayout()  const noexcept { return m_LastOldLayout; }
        RHIImageLayout   GetLastNewLayout()  const noexcept { return m_LastNewLayout; }
        RHIAccessFlags   GetLastSrcAccess()  const noexcept { return m_LastSrcAccess; }
        RHIAccessFlags   GetLastDstAccess()  const noexcept { return m_LastDstAccess; }

    private:
        u32            m_BeginCount     = 0;
        u32            m_EndCount       = 0;
        u32            m_TransitionCount = 0;
        RHITexture*    m_LastTexture    = nullptr;
        RHIImageLayout m_LastOldLayout  = RHIImageLayout::Undefined;
        RHIImageLayout m_LastNewLayout  = RHIImageLayout::Undefined;
        RHIAccessFlags m_LastSrcAccess  = RHIAccessFlags::None;
        RHIAccessFlags m_LastDstAccess  = RHIAccessFlags::None;
    };

    // Fixed-value StubTexture used as a TransitionTexture target.
    // Remains local because it carries test-specific hardcoded properties.
    class StubTexture : public RHITexture
    {
    public:
        StubTexture(RHIDevice& owner) : RHITexture(owner, owner.GetBackend()) {}

        RHIFormat          GetFormat()      const noexcept override { return RHIFormat::D32_FLOAT; }
        RHITextureDimension GetDimension()   const noexcept override { return RHITextureDimension::Texture2D; }
        u32                GetWidth()       const noexcept override { return 1920; }
        u32                GetHeight()      const noexcept override { return 1080; }
        u32                GetDepth()       const noexcept override { return 1; }
        u32                GetMipLevels()   const noexcept override { return 1; }
        u32                GetArrayLayers() const noexcept override { return 1; }
        RHITextureUsage    GetUsage()       const noexcept override { return RHITextureUsage::DepthStencil; }
    };
}}

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;
    using StubDevice   = Test::StubDevice;

    static_assert(std::is_polymorphic_v<RHICommandList>,
                  "RHICommandList must be polymorphic");

    TEST(RHICommandList, BeginEndAreCallable)
    {
        StubInstance instance;
        StubDevice device(instance);
        RecordingCommandList cmdList(device);

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
        StubInstance instance;
        StubDevice device(instance);
        RecordingCommandList cmdList(device);
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
        StubInstance instance;
        StubDevice device(instance);
        RecordingCommandList cmdList(device);

        EXPECT_EQ(cmdList.GetOwnerDevice(), &device);
        EXPECT_EQ(cmdList.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHICommandList, IsPolymorphicThroughBasePointer)
    {
        StubInstance instance;
        StubDevice device(instance);
        RecordingCommandList cmdList(device);

        RHICommandList* base = &cmdList;
        base->Begin();
        EXPECT_EQ(cmdList.GetBeginCount(), 1u);
    }
}
