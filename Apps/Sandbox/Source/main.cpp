#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
