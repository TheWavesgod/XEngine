#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class PlatformEventType
    {
        None,
        WindowClose,
        WindowResize,
        WindowMinimized,
        WindowRestored
    };

    struct PlatformEvent
    {
        PlatformEventType Type = PlatformEventType::None;
        u32 Width = 0;
        u32 Height = 0;
    };
}
