#pragma once

#include <XEngine/Core/Types.h>

#include <string>

namespace XEngine
{
    struct EngineConfig
    {
        std::string ApplicationName = "XEngine";
        bool EnableValidation = true;
        bool EnableEditor = false;
        u32 MaxFrames = 3;
    };
}
