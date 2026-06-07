#include <XEngine/Engine/Engine.h>

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/SubsystemContext.h>
#include <XEngine/Input/InputSystem.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/PlatformSystem.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/Renderer/RenderSystem.h>
#include <XEngine/Scene/SceneSystem.h>
#include <XEngine/Shader/ShaderSystem.h>

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

        if (m_Config.CreateMainWindow)
        {
            m_SubsystemManager.AddSubsystem<PlatformSystem>();
            m_SubsystemManager.AddSubsystem<InputSystem>();
        }

        if (m_Config.EnableShaderCompiler)
        {
            m_SubsystemManager.AddSubsystem<ShaderSystem>();
        }

        m_SubsystemManager.AddSubsystem<AssetSystem>();
        m_SubsystemManager.AddSubsystem<SceneSystem>();

        if (m_Config.CreateGraphicsDevice)
        {
            m_SubsystemManager.AddSubsystem<RHISystem>();
            m_SubsystemManager.AddSubsystem<RenderSystem>();
        }

        SubsystemContext context;
        context.Engine = this;
        context.Config = &m_Config;
        m_SubsystemManager.CreateAll(context);

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
            m_Time.Tick();

            m_SubsystemManager.BeginFrame();
            m_SubsystemManager.Update(m_Time.GetDeltaTime());
            m_SubsystemManager.EndFrame();

            ++frame;

            if (m_Config.MaxFrames > 0 && frame >= m_Config.MaxFrames)
            {
                RequestShutdown();
            }
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
