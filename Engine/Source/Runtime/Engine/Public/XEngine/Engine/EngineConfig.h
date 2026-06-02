#pragma once

#include <string>

namespace XEngine
{
    struct EngineConfig
    {
        std::string ApplicationName = "XEngine";
        bool EnableValidation = true;
        bool EnableEditor = false;
    };
}
