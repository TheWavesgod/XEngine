#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{   
    class RHITextureView;

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

    class RHITexture : public RHIResource
    {
    public:
        ~RHITexture() override = default;

        virtual const RHITextureDesc& GetDesc() const = 0;

        // Default-view accessor: every texture owns one default view that
        // covers all mips and all layers, sampled usage, the texture's
        // primary aspect.
        virtual RHITextureView* GetDefaultView() const = 0;

        virtual void* GetNativeDefaultView(RHIBackend backend) const;

    protected:
        explicit RHITexture(RHIDevice& ownerDevice);
    };
}
