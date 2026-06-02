#pragma once

#include <XEngine/Engine/EngineConfig.h>

namespace XEngine
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        void Initialize(const EngineConfig& config);
        void Run();
        void Shutdown();

        bool IsRunning() const;

    private:
        EngineConfig m_Config;
        bool m_Running = false;
    };
}
