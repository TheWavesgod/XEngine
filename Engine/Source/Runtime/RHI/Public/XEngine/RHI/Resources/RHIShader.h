#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIShader : public RHIResource
    {
    public:
        ~RHIShader() override = default;

        virtual ShaderStage GetStage() const = 0;
        virtual ShaderTarget GetTarget() const = 0;

    protected:
        explicit RHIShader(RHIDevice& ownerDevice);
    };
}
