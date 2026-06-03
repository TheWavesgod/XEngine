#pragma once

#include <XEngine/Core/Types.h>

#include <string>

namespace XEngine
{
    enum class GraphicsBackend
    {
        None,
        Vulkan
    };

    struct EngineConfig
    {
        std::string ApplicationName = "XEngine";

        bool EnableValidation = true;
        bool EnableEditor = false;

        // 0 means run until RequestShutdown().
        u32 MaxFrames = 0;

        u32 WindowWidth = 1280;
        u32 WindowHeight = 720;

        bool WindowResizable = true;
        bool WindowMaximized = false;
        bool CreateMainWindow = true;

        bool CreateGraphicsDevice = true;
        bool EnableVSync = true;
        GraphicsBackend Backend = GraphicsBackend::Vulkan;
    };
}
