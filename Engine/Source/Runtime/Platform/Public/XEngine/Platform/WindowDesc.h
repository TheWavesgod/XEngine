#pragma once

#include <XEngine/Core/Types.h>

#include <string>

namespace XEngine
{
    struct WindowDesc
    {
        std::string Title = "XEngine";
        u32 Width = 1280;
        u32 Height = 720;
        bool Resizable = true;
        bool Maximized = false;
    };
}
