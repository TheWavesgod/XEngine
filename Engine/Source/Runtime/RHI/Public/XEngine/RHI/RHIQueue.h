#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIQueue
    {
    public:
        virtual ~RHIQueue() = default;

        virtual RHIQueueType GetType() const = 0;
    };
}
