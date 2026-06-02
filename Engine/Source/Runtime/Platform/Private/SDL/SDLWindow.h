#pragma once

#include <XEngine/Platform/Window.h>

namespace XEngine
{
    class SDLWindow final : public Window
    {
    public:
        explicit SDLWindow(const WindowDesc& desc);

    private:
        WindowDesc m_Desc;
    };
}

