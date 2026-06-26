#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITexture;

    struct RHITextureViewDesc
    {
        const RHITexture*           Texture = nullptr;
        RHITextureViewUsageFlags    Usage = RHITextureViewUsageFlags::Sampled;
        RHITextureViewDimension     ViewDimension = RHITextureViewDimension::Texture2D;
        
        RHITextureAspectFlags       Aspect = RHITextureAspectFlags::Color;
        
        RHIFormat                   Format = RHIFormat::Undefined;
        
        u32                         BaseMipLevel = 0;
        u32                         MipCount = 1;       // 0 means "all remaining mips" — Stage 7 validation
        u32                         BaseArrayLayer = 0;
        u32                         ArrayLayerCount = 1; // 0 means "all remaining layers"
        
        const char*                 DebugName = nullptr;
    };

    class RHITextureView : public RHIResource
    {
    public:
        ~RHITextureView() override = default;

        virtual const RHITextureViewDesc& GetDesc() const = 0;
        const RHITexture* GetTexture() const;

        // Backend-specific native image view handle. Returns nullptr for
        // backends whose handles are owned by the backend object itself.
        virtual void* GetNativeView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }

    protected:
        explicit RHITextureView(RHIDevice& ownerDevice);
    };
}
