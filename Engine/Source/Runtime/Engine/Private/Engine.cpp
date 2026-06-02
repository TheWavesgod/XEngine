#include <XEngine/Engine/Engine.h>

#include <XEngine/Logging/Log.h>

namespace XEngine
{
    Engine::Engine() = default;

    Engine::~Engine()
    {
        if (m_Running)
        {
            Shutdown();
        }
    }

    void Engine::Initialize(const EngineConfig& config)
    {
        m_Config = config;
        m_Running = true;
        XENGINE_LOG_INFO("XEngine initialized");
    }

    void Engine::Run()
    {
        if (!m_Running)
        {
            return;
        }

        XENGINE_LOG_INFO("XEngine placeholder run loop");
    }

    void Engine::Shutdown()
    {
        if (!m_Running)
        {
            return;
        }

        XENGINE_LOG_INFO("XEngine shutdown");
        m_Running = false;
    }

    bool Engine::IsRunning() const
    {
        return m_Running;
    }
}
