#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITexture;

    enum class RHITextureViewDimension : u8
    {
        Texture2D,
        Texture2DArray,
        TextureCube,
    };

    enum class RHITextureAspectFlags : u8
    {
        None     = 0,
        Color    = 1 << 0,
        Depth    = 1 << 1,
        Stencil  = 1 << 2,
        DepthStencil = Depth | Stencil,
    };

    inline RHITextureAspectFlags operator| (RHITextureAspectFlags lhs, RHITextureAspectFlags rhs)
    {
        return static_cast<RHITextureAspectFlags>(
            static_cast<u8>(lhs) | static_cast<u8>(rhs));
    } 

    inline bool HasFlag(RHITextureAspectFlags value, RHITextureAspectFlags flag)
    {
        return (static_cast<u8>(value) & static_cast<u8>(flag)) != 0;
    } 

    struct RHITextureViewDesc
    {
        const RHITexture*       Texture = nullptr;
        RHITextureViewDimension ViewDimension = RHITextureViewDimension::Texture2D;
        RHIFormat               Format = RHIFormat::Undefined;
        u32                     BaseMipLevel = 0;
        u32                     MipCount = 1;
        u32                     BaseArrayLayer = 0;
        u32                     ArrayLayerCount = 1;
        RHITextureAspectFlags   Aspect = RHITextureAspectFlags::Color;
        const char*             DebugName = nullptr;
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
