#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITexture : public RHIResource
    {
    public:
        ~RHITexture() override = default;

        virtual const RHITextureDesc& GetDesc() const = 0;

    protected:
        explicit RHITexture(RHIDevice& ownerDevice);
    };
}
