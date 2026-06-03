#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;

        virtual bool IsValid() const = 0;

        virtual void BeginFrame() = 0;
        virtual void ClearSwapchain(const RHIColor& color) = 0;
        virtual void EndFrame() = 0;

        virtual void RequestResize(u32 width, u32 height) = 0;

        virtual void WaitIdle() = 0;
    };
}
