#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Input/InputTypes.h>

namespace XEngine
{
    enum class PlatformEventType
    {
        None,
        WindowClose,
        WindowResize,
        WindowMinimized,
        WindowRestored,
        WindowFocusGained,
        WindowFocusLost,
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseWheel
    };

    // Engine-level event emitted by Platform backends.
    // This public type intentionally contains no SDL/native backend types.
    struct PlatformEvent
    {
        PlatformEventType Type = PlatformEventType::None;
        u32 Width = 0;
        u32 Height = 0;
        KeyCode Key = KeyCode::Unknown;
        bool Repeat = false;
        MouseButton Button = MouseButton::Left;
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        float MouseDeltaX = 0.0f;
        float MouseDeltaY = 0.0f;
        float WheelDeltaX = 0.0f;
        float WheelDeltaY = 0.0f;
    };
}
