#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    struct RHITextureDesc
    {
        u32 Width = 1;
        u32 Height = 1;
        u32 MipLevels = 1;
        u32 ArrayLayers = 1;

        RHIFormat Format = RHIFormat::RGBA8Unorm;
        RHITextureDimension Dimension = RHITextureDimension::Texture2D;
        RHITextureUsageFlags Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;

        bool GenerateMips = false;
        const char* DebugName = nullptr;
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;

        virtual const RHITextureDesc& GetDesc() const = 0;
    };
}
