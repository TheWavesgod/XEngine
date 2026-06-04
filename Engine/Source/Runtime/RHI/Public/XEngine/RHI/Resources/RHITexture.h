#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    enum class RHITextureUsage : u32
    {
        None = 0,
        ColorAttachment = 1 << 0,
        DepthStencil = 1 << 1,
        Sampled = 1 << 2,
        TransferSrc = 1 << 3,
        TransferDst = 1 << 4
    };

    inline RHITextureUsage operator|(RHITextureUsage lhs, RHITextureUsage rhs)
    {
        return static_cast<RHITextureUsage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHITextureUsage value, RHITextureUsage flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    struct RHITextureDesc
    {
        u32 Width = 0;
        u32 Height = 0;
        RHIFormat Format = RHIFormat::Undefined;
        RHITextureUsage Usage = RHITextureUsage::None;
        const char* DebugName = nullptr;
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;
        virtual RHIFormat GetFormat() const = 0;
    };
}
