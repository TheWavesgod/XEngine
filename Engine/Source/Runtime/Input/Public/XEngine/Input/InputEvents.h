#pragma once

#include <XEngine/Input/InputTypes.h>

namespace XEngine
{
    struct KeyEvent
    {
        KeyCode Key = KeyCode::Unknown;
        bool Repeat = false;
    };
}
