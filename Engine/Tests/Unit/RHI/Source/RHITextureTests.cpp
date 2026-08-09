// Unit tests for RHITexture abstract interface.
//
// M5 surface: 8 getter methods (GetFormat / GetDimension / Width / Height /
// Depth / MipLevels / ArrayLayers / GetUsage). Tests verify the virtual
// dispatch contract via a stub device that creates in-memory textures.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIFlags.h>

#include "RHITestStubs.h"

#include <memory>
#include <vector>

namespace XEngine
{
    // Stub texture — records info from desc. Local because it carries
    // test-specific stored desc state.
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

    // TextureTestDevice — derives from the shared Test::StubDevice and
    // only overrides the texture factory hook.
    class TextureTestDevice final : public Test::StubDevice
    {
    public:
        explicit TextureTestDevice(RHIInstance& owner)
            : Test::StubDevice(owner)
        {
        }

        RHITexture* CreateTextureImpl(const RHITextureDesc& desc) override
        {
            m_LastTexture = std::make_unique<StubTexture>(*this, desc);
            return m_LastTexture.get();
        }

        StubTexture* GetLastTexture() noexcept { return m_LastTexture.get(); }

    private:
        std::unique_ptr<StubTexture> m_LastTexture;
    };
}

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;

    static_assert(std::is_polymorphic_v<RHITexture>,
                  "RHITexture must be polymorphic");

    TEST(RHITexture, GettersReflectDesc)
    {
        StubInstance instance;
        TextureTestDevice device(instance);

        RHITextureDesc desc{
            .Dimension = RHITextureDimension::Texture2D,
            .Width = 1920,
            .Height = 1080,
            .Depth = 1,
            .MipLevels = 1,
            .ArrayLayers = 1,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead | RHITextureUsage::RenderTarget,
        };
        RHITexture* tex = device.CreateTexture(desc);
        ASSERT_NE(tex, nullptr);

        EXPECT_EQ(tex->GetFormat(), RHIFormat::R8G8B8A8_UNORM);
        EXPECT_EQ(tex->GetDimension(), RHITextureDimension::Texture2D);
        EXPECT_EQ(tex->GetWidth(), 1920u);
        EXPECT_EQ(tex->GetHeight(), 1080u);
        EXPECT_EQ(tex->GetDepth(), 1u);
        EXPECT_EQ(tex->GetMipLevels(), 1u);
        EXPECT_EQ(tex->GetArrayLayers(), 1u);
        EXPECT_EQ(tex->GetUsage(), RHITextureUsage::ShaderRead | RHITextureUsage::RenderTarget);
    }

    TEST(RHITexture, OwnerDeviceIsTheDeviceItCameFrom)
    {
        StubInstance instance;
        TextureTestDevice device(instance);

        RHITexture* tex = device.CreateTexture({
            .Width = 64, .Height = 64,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        });
        ASSERT_NE(tex, nullptr);

        EXPECT_EQ(tex->GetOwnerDevice(), &device);
        EXPECT_EQ(tex->GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHITexture, CubeArrayDimension)
    {
        // Audit 3.10 — TextureCubeArray must be supported.
        StubInstance instance;
        TextureTestDevice device(instance);

        RHITexture* tex = device.CreateTexture({
            .Dimension = RHITextureDimension::TextureCubeArray,
            .Width = 512, .Height = 512,
            .ArrayLayers = 12,  // 2 cubemaps
            .Format = RHIFormat::R16G16B16A16_FLOAT,
            .Usage = RHITextureUsage::ShaderRead,
        });
        ASSERT_NE(tex, nullptr);

        EXPECT_EQ(tex->GetDimension(), RHITextureDimension::TextureCubeArray);
        EXPECT_EQ(tex->GetArrayLayers(), 12u);
    }

    TEST(RHITexture, MipLevelsAndArrayLayersCorrect)
    {
        StubInstance instance;
        TextureTestDevice device(instance);

        RHITexture* tex = device.CreateTexture({
            .Width = 1024, .Height = 1024,
            .MipLevels = 11,  // log2(1024) + 1
            .ArrayLayers = 4,
            .Format = RHIFormat::R8G8B8A8_SRGB,
            .Usage = RHITextureUsage::ShaderRead,
        });
        ASSERT_NE(tex, nullptr);

        EXPECT_EQ(tex->GetMipLevels(), 11u);
        EXPECT_EQ(tex->GetArrayLayers(), 4u);
    }

    TEST(RHITexture, IsPolymorphicThroughBasePointer)
    {
        StubInstance instance;
        TextureTestDevice device(instance);

        RHITexture* tex = device.CreateTexture({
            .Width = 256, .Height = 256,
            .Format = RHIFormat::R32_FLOAT,
            .Usage = RHITextureUsage::ShaderRead | RHITextureUsage::ShaderWrite,
        });
        ASSERT_NE(tex, nullptr);

        RHITexture* base = tex;
        EXPECT_EQ(base->GetWidth(), 256u);
        EXPECT_EQ(base->GetFormat(), RHIFormat::R32_FLOAT);
    }
}
