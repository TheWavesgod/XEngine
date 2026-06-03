#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Editor";
    config.EnableEditor = true;
    config.WindowWidth = 1600;
    config.WindowHeight = 900;
    config.WindowResizable = true;
    config.CreateMainWindow = true;
    config.MaxFrames = 0;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
