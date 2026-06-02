#pragma once

#include <XEngine/Platform/NativeWindowHandle.h>

#include <string>

namespace XEngine
{
    struct WindowDesc
    {
        std::string Title = "XEngine";
        int Width = 1280;
        int Height = 720;
    };

    class Window
    {
    public:
        virtual ~Window() = default;
        virtual void PollEvents() {}
        virtual NativeWindowHandle GetNativeHandle() const { return {}; }
    };
}
