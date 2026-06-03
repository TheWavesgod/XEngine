#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/Platform/WindowDesc.h>

#include <string_view>

namespace XEngine
{
    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void PollEvents() = 0;

        virtual bool ShouldClose() const = 0;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;

        virtual std::string_view GetTitle() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };
}
