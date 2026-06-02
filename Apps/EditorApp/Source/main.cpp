#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Editor";
    config.EnableEditor = true;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
