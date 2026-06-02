#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";
    config.MaxFrames = 3;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
