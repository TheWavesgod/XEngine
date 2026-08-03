#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHISampler : public RHIResource
    {
    public:
        ~RHISampler() override = default;

        virtual const RHISamplerDesc& GetDesc() const = 0;

    protected:
        explicit RHISampler(RHIDevice& ownerDevice);
    };
}
