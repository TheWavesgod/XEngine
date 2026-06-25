#pragma once

#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIResource.h>

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

    class RHISampler : public RHIResource
    {
    public:
        ~RHISampler() override = default;

        virtual const RHISamplerDesc& GetDesc() const = 0;
        virtual void* GetNativeSampler(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }

    protected:
        explicit RHISampler(RHIDevice& ownerDevice);
    };
}
