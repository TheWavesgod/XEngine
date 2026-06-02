#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    template<typename Tag>
    struct Handle
    {
        u32 Index = 0xFFFFFFFFu;
        u32 Generation = 0;
    };
}

