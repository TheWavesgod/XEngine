// Unit tests for RHIValidation desc validators.
//
// M4 surface: ValidateBufferDesc. Tests verify both success and failure
// paths independently of any RHIDevice / RHIBuffer instance.

#include <gtest/gtest.h>

#include <XEngine/Core/Result.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIValidation.h>
#include <XEngine/RHI/RHIFlags.h>

namespace
{
    using namespace XEngine;

    // ---------------------------------------------------------------------
    // ValidateBufferDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, BufferDescSizeZeroIsFailure)
    {
        RHIBufferDesc desc{
            .Size = 0,
            .Usage = RHIBufferUsage::Vertex,
        };

        auto result = ValidateBufferDesc(desc);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.Message.empty());
    }

    TEST(RHIValidation, BufferDescUsageNoneIsFailure)
    {
        RHIBufferDesc desc{
            .Size = 1024,
            .Usage = RHIBufferUsage::None,
        };

        auto result = ValidateBufferDesc(desc);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.Message.empty());
    }

    TEST(RHIValidation, BufferDescEmptyIsFailure)
    {
        RHIBufferDesc desc;  // all defaults

        auto result = ValidateBufferDesc(desc);
        EXPECT_FALSE(result);
    }

    TEST(RHIValidation, BufferDescSingleFlagIsSuccess)
    {
        RHIBufferDesc descs[] = {
            { .Size = 64,   .Usage = RHIBufferUsage::Vertex },
            { .Size = 128,  .Usage = RHIBufferUsage::Index },
            { .Size = 256,  .Usage = RHIBufferUsage::Uniform },
            { .Size = 1024, .Usage = RHIBufferUsage::Storage },
            { .Size = 512,  .Usage = RHIBufferUsage::TransferSrc },
            { .Size = 2048, .Usage = RHIBufferUsage::TransferDst },
            { .Size = 64,   .Usage = RHIBufferUsage::Indirect },
        };

        for (const auto& desc : descs)
        {
            auto result = ValidateBufferDesc(desc);
            EXPECT_TRUE(result) << "Size=" << desc.Size << " Usage=" << static_cast<u32>(desc.Usage);
        }
    }

    TEST(RHIValidation, BufferDescMultipleFlagsIsSuccess)
    {
        RHIBufferDesc desc{
            .Size = 1024,
            .Usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst,
        };

        auto result = ValidateBufferDesc(desc);
        EXPECT_TRUE(result);
    }

    TEST(RHIValidation, BufferDescTypicalBackendUsageIsSuccess)
    {
        // Vertex/index + transfer dst (typical per-frame upload pattern)
        RHIBufferDesc vertexBuffer{
            .Size = 1024 * 1024,
            .Usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst,
        };
        RHIBufferDesc indexBuffer{
            .Size = 1024 * 1024,
            .Usage = RHIBufferUsage::Index | RHIBufferUsage::TransferDst,
        };
        RHIBufferDesc uniformBuffer{
            .Size = 1024,
            .Usage = RHIBufferUsage::Uniform,
        };
        RHIBufferDesc storageBuffer{
            .Size = 1024 * 1024,
            .Usage = RHIBufferUsage::Storage | RHIBufferUsage::TransferDst,
        };

        EXPECT_TRUE(ValidateBufferDesc(vertexBuffer));
        EXPECT_TRUE(ValidateBufferDesc(indexBuffer));
        EXPECT_TRUE(ValidateBufferDesc(uniformBuffer));
        EXPECT_TRUE(ValidateBufferDesc(storageBuffer));
    }

    // ---------------------------------------------------------------------
    // ValidateTextureDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, TextureDescEmptyIsFailure)
    {
        RHITextureDesc desc;  // all defaults
        auto result = ValidateTextureDesc(desc);
        EXPECT_FALSE(result);
    }

    TEST(RHIValidation, TextureDescFormatUnknownIsFailure)
    {
        RHITextureDesc desc{
            .Width = 64, .Height = 64,
            .Format = RHIFormat::Unknown,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescUsageNoneIsFailure)
    {
        RHITextureDesc desc{
            .Width = 64, .Height = 64,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::None,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescWidthZeroIsFailure)
    {
        RHITextureDesc desc{
            .Width = 0, .Height = 64,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescHeightZeroNon1DIsFailure)
    {
        RHITextureDesc desc{
            .Width = 64, .Height = 0,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescHeightZero1DIsSuccess)
    {
        RHITextureDesc desc{
            .Dimension = RHITextureDimension::Texture1D,
            .Width = 64, .Height = 0,
            .Format = RHIFormat::R8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_TRUE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescDepthZero3DIsFailure)
    {
        RHITextureDesc desc{
            .Dimension = RHITextureDimension::Texture3D,
            .Width = 64, .Height = 64, .Depth = 0,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescMipLevelsZeroIsFailure)
    {
        RHITextureDesc desc{
            .Width = 64, .Height = 64,
            .MipLevels = 0,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescArrayLayersZeroIsFailure)
    {
        RHITextureDesc desc{
            .Width = 64, .Height = 64,
            .ArrayLayers = 0,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::ShaderRead,
        };
        EXPECT_FALSE(ValidateTextureDesc(desc));
    }

    TEST(RHIValidation, TextureDescTypicalBackendUsageIsSuccess)
    {
        RHITextureDesc colorAttachment{
            .Dimension = RHITextureDimension::Texture2D,
            .Width = 1920, .Height = 1080,
            .Format = RHIFormat::R8G8B8A8_UNORM,
            .Usage = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderRead,
        };
        RHITextureDesc depthAttachment{
            .Dimension = RHITextureDimension::Texture2D,
            .Width = 1920, .Height = 1080,
            .Format = RHIFormat::D32_FLOAT,
            .Usage = RHITextureUsage::DepthStencil | RHITextureUsage::ShaderRead,
        };
        RHITextureDesc cubeMap{
            .Dimension = RHITextureDimension::TextureCube,
            .Width = 512, .Height = 512,
            .ArrayLayers = 6,
            .Format = RHIFormat::R16G16B16A16_FLOAT,
            .Usage = RHITextureUsage::ShaderRead,
        };
        RHITextureDesc cubeMapArray{
            .Dimension = RHITextureDimension::TextureCubeArray,
            .Width = 512, .Height = 512,
            .ArrayLayers = 12,
            .Format = RHIFormat::R16G16B16A16_FLOAT,
            .Usage = RHITextureUsage::ShaderRead,
        };

        EXPECT_TRUE(ValidateTextureDesc(colorAttachment));
        EXPECT_TRUE(ValidateTextureDesc(depthAttachment));
        EXPECT_TRUE(ValidateTextureDesc(cubeMap));
        EXPECT_TRUE(ValidateTextureDesc(cubeMapArray));
    }

    // ---------------------------------------------------------------------
    // ValidateTextureViewDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, TextureViewDescSourceNullIsFailure)
    {
        RHITextureViewDesc desc{
            .Source = nullptr,
            .Format = RHIFormat::R8G8B8A8_UNORM,
        };
        EXPECT_FALSE(ValidateTextureViewDesc(desc));
    }

    TEST(RHIValidation, TextureViewDescFormatUnknownIsFailure)
    {
        // We need a valid source for the check to reach the format check.
        // For this test we provide a non-null source (a pointer to a stub
        // is overkill — any non-null pointer works since the validator only
        // checks for null).
        struct StubTexture {
            char dummy = 0;
        };
        RHITextureViewDesc desc{
            .Source = reinterpret_cast<const RHITexture*>(0x1),
            .Format = RHIFormat::Unknown,
        };
        EXPECT_FALSE(ValidateTextureViewDesc(desc));
        (void)sizeof(StubTexture);
    }

    TEST(RHIValidation, TextureViewDescValidIsSuccess)
    {
        // Source must be non-null for the check to succeed.
        RHITextureViewDesc desc{
            .Source = reinterpret_cast<const RHITexture*>(0x1),
            .Format = RHIFormat::R8G8B8A8_SRGB,
        };
        EXPECT_TRUE(ValidateTextureViewDesc(desc));
    }

    // ---------------------------------------------------------------------
    // ValidateSamplerDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, SamplerDescDefaultIsSuccess)
    {
        EXPECT_TRUE(ValidateSamplerDesc(RHISamplerDesc{}));
    }

    TEST(RHIValidation, SamplerDescPCFCompareIsSuccess)
    {
        // Audit 3.1 — PCF (sampler with CompareEnable) must be supported.
        RHISamplerDesc pcf{
            .CompareEnable = true,
            .CompareOp = RHICompareOp::LessEqual,
        };
        EXPECT_TRUE(ValidateSamplerDesc(pcf));
    }

    // ---------------------------------------------------------------------
    // ValidateFenceDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, FenceDescDefaultIsSuccess)
    {
        EXPECT_TRUE(ValidateFenceDesc(RHIFenceDesc{}));
    }

    TEST(RHIValidation, FenceDescInitialSignaledIsSuccess)
    {
        RHIFenceDesc fence{ .InitialSignaled = true };
        EXPECT_TRUE(ValidateFenceDesc(fence));
    }

    // ---------------------------------------------------------------------
    // ValidateSemaphoreDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, SemaphoreDescDefaultIsSuccess)
    {
        EXPECT_TRUE(ValidateSemaphoreDesc(RHISemaphoreDesc{}));
    }

    // ---------------------------------------------------------------------
    // ValidateCommandListDesc
    // ---------------------------------------------------------------------

    TEST(RHIValidation, CommandListDescDefaultIsSuccess)
    {
        RHICommandListDesc cmdList;
        EXPECT_TRUE(ValidateCommandListDesc(cmdList));
    }

    TEST(RHIValidation, CommandListDescAllQueueTypesAreSuccess)
    {
        EXPECT_TRUE(ValidateCommandListDesc({.TargetQueue = RHIQueueType::Graphics}));
        EXPECT_TRUE(ValidateCommandListDesc({.TargetQueue = RHIQueueType::Compute}));
        EXPECT_TRUE(ValidateCommandListDesc({.TargetQueue = RHIQueueType::Transfer}));
    }
}
