#include <XEngine/Platform/PlatformSystem.h>

#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/EngineConfig.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/Window.h>
#include <XEngine/Platform/WindowDesc.h>

#if defined(XENGINE_ENABLE_SDL)
    #include "SDL/SDLPlatformUtils.h"
    #include "SDL/SDLWindow.h"
#endif

#include <string>

namespace XEngine
{
    PlatformSystem::PlatformSystem() = default;

    PlatformSystem::~PlatformSystem()
    {
        OnDestroy();
    }

    void PlatformSystem::OnCreate(const SubsystemContext& context)
    {
        m_Engine = context.Engine;

#if defined(XENGINE_ENABLE_SDL)
        if (!SDLPlatformUtils::InitializeVideo())
        {
            XENGINE_LOG_ERROR("Failed to initialize SDL video subsystem");
            return;
        }

        m_Initialized = true;

        WindowDesc desc;
        if (context.Config != nullptr)
        {
            desc.Title = context.Config->ApplicationName;
            desc.Width = context.Config->WindowWidth;
            desc.Height = context.Config->WindowHeight;
            desc.Resizable = context.Config->WindowResizable;
            desc.Maximized = context.Config->WindowMaximized;
        }

        std::string message = "Creating SDL window: ";
        message += desc.Title;
        message += " ";
        message += std::to_string(desc.Width);
        message += "x";
        message += std::to_string(desc.Height);
        XENGINE_LOG_INFO(message);

        m_MainWindow = std::make_unique<SDLWindow>(desc);
#else
        XENGINE_LOG_WARN("SDL platform backend is disabled. No real platform window will be created.");
#endif
    }

    void PlatformSystem::OnDestroy()
    {
        if (!m_Initialized && !m_MainWindow)
        {
            return;
        }

        m_MainWindow.reset();

#if defined(XENGINE_ENABLE_SDL)
        if (m_Initialized)
        {
            SDLPlatformUtils::QuitVideo();
        }
#endif

        m_Engine = nullptr;
        m_Initialized = false;
    }

    void PlatformSystem::OnBeginFrame()
    {
        if (!m_MainWindow)
        {
            return;
        }

        m_MainWindow->PollEvents();

        if (m_MainWindow->ShouldClose())
        {
            XENGINE_LOG_INFO("Window close requested");

            if (m_Engine != nullptr)
            {
                m_Engine->RequestShutdown();
            }
        }
    }

    Window* PlatformSystem::GetMainWindow()
    {
        return m_MainWindow.get();
    }

    const Window* PlatformSystem::GetMainWindow() const
    {
        return m_MainWindow.get();
    }
}
