// Unit tests for RHITextureView abstract interface.
//
// M5 surface: 7 getter methods (GetFormat / GetSource / GetDimension /
// BaseMipLevel / MipLevelCount / BaseArrayLayer / ArrayLayerCount). Tests
// verify the virtual dispatch contract via a stub device.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class TextureViewTestInstance : public RHIInstance
    {
    public:
        TextureViewTestInstance()
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

    // Stub texture view — records fields from desc.
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

    // Stub texture — needed as source for views.
    class StubTexture : public RHITexture
    {
    public:
        StubTexture(RHIDevice& owner)
            : RHITexture(owner, owner.GetBackend())
        {
        }

        RHIFormat          GetFormat()      const noexcept override { return RHIFormat::R8G8B8A8_UNORM; }
        RHITextureDimension GetDimension() const noexcept override { return RHITextureDimension::Texture2D; }
        u32                GetWidth()      const noexcept override { return 256; }
        u32                GetHeight()     const noexcept override { return 256; }
        u32                GetDepth()      const noexcept override { return 1; }
        u32                GetMipLevels()  const noexcept override { return 1; }
        u32                GetArrayLayers() const noexcept override { return 1; }
        RHITextureUsage    GetUsage()      const noexcept override { return RHITextureUsage::ShaderRead; }
    };

    class TextureViewTestDevice : public RHIDevice
    {
    public:
        explicit TextureViewTestDevice(RHIInstance& owner)
            : RHIDevice(owner, RHIBackend::Vulkan)
        {
        }

        void WaitIdle() override {}
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIQueue* GetQueue(RHIQueueType) const override { return nullptr; }
        RHIBuffer* CreateBufferImpl(const RHIBufferDesc&) override { return nullptr; }
        RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc& desc) override
        {
            m_LastView = std::make_unique<StubTextureView>(*this, desc);
            return m_LastView.get();
        }
        RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        StubTextureView* GetLastView() noexcept { return m_LastView.get(); }

    private:
        RHICapabilities m_Caps;
        std::unique_ptr<StubTextureView> m_LastView;
    };
}

namespace
{
    using namespace XEngine;

    static_assert(std::is_polymorphic_v<RHITextureView>,
                  "RHITextureView must be polymorphic");

    TEST(RHITextureView, GettersReflectDesc)
    {
        TextureViewTestInstance instance;
        TextureViewTestDevice device(instance);
        StubTexture source(device);

        RHITextureViewDesc desc{
            .Source = &source,
            .Dimension = RHITextureDimension::Texture2D,
            .Format = RHIFormat::R8G8B8A8_SRGB,
            .BaseMipLevel = 1,
            .MipLevelCount = 4,
            .BaseArrayLayer = 0,
            .ArrayLayerCount = 2,
        };
        RHITextureView* view = device.CreateTextureView(desc);
        ASSERT_NE(view, nullptr);

        EXPECT_EQ(view->GetSource(), &source);
        EXPECT_EQ(view->GetFormat(), RHIFormat::R8G8B8A8_SRGB);
        EXPECT_EQ(view->GetDimension(), RHITextureDimension::Texture2D);
        EXPECT_EQ(view->GetBaseMipLevel(), 1u);
        EXPECT_EQ(view->GetMipLevelCount(), 4u);
        EXPECT_EQ(view->GetBaseArrayLayer(), 0u);
        EXPECT_EQ(view->GetArrayLayerCount(), 2u);
    }

    TEST(RHITextureView, MipAndArrayCountZeroMeansAllRemaining)
    {
        // Vulkan convention: 0 in MipLevelCount / ArrayLayerCount means
        // all remaining from BaseMipLevel / BaseArrayLayer.
        TextureViewTestInstance instance;
        TextureViewTestDevice device(instance);
        StubTexture source(device);

        RHITextureViewDesc desc{
            .Source = &source,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .BaseMipLevel = 2,
            .MipLevelCount = 0,  // all remaining
            .BaseArrayLayer = 1,
            .ArrayLayerCount = 0,  // all remaining
        };
        RHITextureView* view = device.CreateTextureView(desc);
        ASSERT_NE(view, nullptr);

        EXPECT_EQ(view->GetBaseMipLevel(), 2u);
        EXPECT_EQ(view->GetMipLevelCount(), 0u);
        EXPECT_EQ(view->GetBaseArrayLayer(), 1u);
        EXPECT_EQ(view->GetArrayLayerCount(), 0u);
    }

    TEST(RHITextureView, OwnerDeviceIsTheDeviceItCameFrom)
    {
        TextureViewTestInstance instance;
        TextureViewTestDevice device(instance);
        StubTexture source(device);

        RHITextureView* view = device.CreateTextureView({
            .Source = &source,
            .Format = RHIFormat::R8G8B8A8_UNORM,
        });
        ASSERT_NE(view, nullptr);

        EXPECT_EQ(view->GetOwnerDevice(), &device);
        EXPECT_EQ(view->GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHITextureView, IsPolymorphicThroughBasePointer)
    {
        TextureViewTestInstance instance;
        TextureViewTestDevice device(instance);
        StubTexture source(device);

        RHITextureView* view = device.CreateTextureView({
            .Source = &source,
            .Format = RHIFormat::R8G8B8A8_UNORM,
        });
        ASSERT_NE(view, nullptr);

        RHITextureView* base = view;
        EXPECT_EQ(base->GetSource(), &source);
        EXPECT_EQ(base->GetFormat(), RHIFormat::R8G8B8A8_UNORM);
    }
}
