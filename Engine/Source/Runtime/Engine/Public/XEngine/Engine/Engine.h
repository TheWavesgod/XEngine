#pragma once

#include <XEngine/Engine/EngineConfig.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Engine/Time.h>

namespace XEngine
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void Initialize(const EngineConfig& config);
        void Run();
        void Shutdown();

        void RequestShutdown();

        bool IsRunning() const;

        SubsystemManager& GetSubsystemManager();
        const Time& GetTime() const;

    private:
        EngineConfig m_Config;
        SubsystemManager m_SubsystemManager;
        Time m_Time;

        bool m_Initialized = false;
        bool m_Running = false;
    };
}
