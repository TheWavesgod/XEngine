#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    class RHISwapchain
    {
    public:
        virtual ~RHISwapchain() = default;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;
    };
}
