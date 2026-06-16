#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/Platform/PlatformEvents.h>
#include <XEngine/Platform/WindowDesc.h>

#include <string_view>
#include <vector>

namespace XEngine
{
    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void PollEvents(std::vector<PlatformEvent>& events) = 0;

        virtual bool ShouldClose() const = 0;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;
        virtual void SetCursorVisible(bool visible) = 0;
        virtual void SetRelativeMouseMode(bool enabled) = 0;
        virtual bool IsFocused() const = 0;

        virtual std::string_view GetTitle() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };
}
