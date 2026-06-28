#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIBindGroupLayout : public RHIResource
    {
    public:
        ~RHIBindGroupLayout() override = default;

        virtual const RHIBindGroupLayoutDesc& GetDesc() const = 0;

    protected:
        explicit RHIBindGroupLayout(RHIDevice& ownerDevice);
    };

    class RHIBindGroup : public RHIResource
    {
    public:
        ~RHIBindGroup() override = default;

        virtual const RHIBindGroupDesc& GetDesc() const = 0;

    protected:
        explicit RHIBindGroup(RHIDevice& ownerDevice);
    };
}
