#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITextureView : public RHIResource
    {
    public:
        ~RHITextureView() override = default;

        virtual const RHITextureViewDesc& GetDesc() const = 0;
        RHITexture* GetTexture() const;

    protected:
        explicit RHITextureView(RHIDevice& ownerDevice);
    };
}
