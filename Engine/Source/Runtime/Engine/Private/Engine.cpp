#include <XEngine/Engine/Engine.h>

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    Engine::Engine() = default;

    Engine::~Engine()
    {
        if (m_Initialized)
        {
            Shutdown();
        }
    }

    void Engine::Initialize(const EngineConfig& config)
    {
        XENGINE_ASSERT(!m_Initialized, "Engine is already initialized");
        if (m_Initialized)
        {
            return;
        }

        m_Config = config;

        Log::Initialize();

        std::string message = "Initializing engine: ";
        message += m_Config.ApplicationName;
        XENGINE_LOG_INFO(message);

        m_Time.Reset();
        m_SubsystemManager.CreateAll();

        m_Initialized = true;
        XENGINE_LOG_INFO("Engine initialized");
    }

    void Engine::Run()
    {
        XENGINE_ASSERT(m_Initialized, "Engine must be initialized before Run");
        if (!m_Initialized)
        {
            return;
        }

        m_Running = true;
        XENGINE_LOG_INFO("Engine started");

        u32 frame = 0;
        while (m_Running)
        {
            if (frame >= m_Config.MaxFrames)
            {
                RequestShutdown();
                break;
            }

            m_Time.Tick();

            m_SubsystemManager.BeginFrame();
            m_SubsystemManager.Update(m_Time.GetDeltaTime());
            m_SubsystemManager.EndFrame();

            std::string frameMessage = "Frame ";
            frameMessage += std::to_string(frame);
            XENGINE_LOG_INFO(frameMessage);

            ++frame;
        }

        XENGINE_LOG_INFO("Engine stopped");
    }

    void Engine::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        XENGINE_LOG_INFO("Engine shutting down");
        RequestShutdown();

        m_SubsystemManager.DestroyAll();

        XENGINE_LOG_INFO("Engine shutdown complete");
        Log::Shutdown();

        m_Initialized = false;
    }

    void Engine::RequestShutdown()
    {
        m_Running = false;
    }

    bool Engine::IsRunning() const
    {
        return m_Running;
    }

    SubsystemManager& Engine::GetSubsystemManager()
    {
        return m_SubsystemManager;
    }

    const Time& Engine::GetTime() const
    {
        return m_Time;
    }
}
