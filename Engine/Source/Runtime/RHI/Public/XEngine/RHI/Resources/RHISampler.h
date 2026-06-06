#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    struct RHISamplerDesc
    {
        RHIFilter MinFilter = RHIFilter::Linear;
        RHIFilter MagFilter = RHIFilter::Linear;

        RHIAddressMode AddressU = RHIAddressMode::Repeat;
        RHIAddressMode AddressV = RHIAddressMode::Repeat;
        RHIAddressMode AddressW = RHIAddressMode::Repeat;

        float MaxAnisotropy = 1.0f;

        const char* DebugName = nullptr;
    };

    class RHISampler
    {
    public:
        virtual ~RHISampler() = default;

        virtual const RHISamplerDesc& GetDesc() const = 0;
    };
}
