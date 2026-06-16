#include <XEngine/Editor/EditorApplication.h>

#include <XEngine/Editor/EditorSystem.h>

namespace XEngine
{
    void EditorApplication::Run()
    {
        EngineConfig config;
        config.ApplicationName = "XEngine Editor";
        config.EnableEditor = true;
        config.WindowWidth = 1600;
        config.WindowHeight = 900;
        config.WindowResizable = true;
        config.CreateMainWindow = true;
        config.MaxFrames = 0;
        config.ConfigureSubsystems = [](SubsystemManager& subsystems)
        {
            subsystems.AddSubsystem<EditorSystem>();
        };

        m_Engine.Initialize(config);
        m_Engine.Run();
        m_Engine.Shutdown();
    }
}
