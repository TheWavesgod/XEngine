#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;

        virtual bool IsValid() const = 0;

        virtual void WaitIdle() = 0;
    };
}
