#pragma once

namespace XEngine
{
    class Engine;
    struct EngineConfig;

    struct SubsystemContext
    {
        Engine* Engine = nullptr;
        const EngineConfig* Config = nullptr;
    };
}
