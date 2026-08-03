#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIPipeline : public RHIResource
    {
    public:
        ~RHIPipeline() override = default;

    protected:
        explicit RHIPipeline(RHIDevice& ownerDevice);
    };
}
