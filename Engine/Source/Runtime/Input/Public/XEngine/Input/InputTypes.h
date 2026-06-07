#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    // Engine-level keyboard codes consumed by InputSystem.
    // Native backend codes are translated in Platform private code.
    enum class KeyCode
    {
        Unknown,
        W,
        A,
        S,
        D,
        Q,
        E,
        LeftShift,
        RightShift,
        Escape,
        Count
    };

    // Engine-level mouse button identifiers.
    enum class MouseButton
    {
        Left,
        Right,
        Middle,
        Count
    };
}
