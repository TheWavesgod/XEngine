#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";
    config.WindowWidth = 1280;
    config.WindowHeight = 720;
    config.WindowResizable = true;
    config.CreateMainWindow = true;
    config.MaxFrames = 0;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
