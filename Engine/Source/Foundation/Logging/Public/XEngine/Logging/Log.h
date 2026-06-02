#pragma once

#include <iostream>

#define XENGINE_LOG_INFO(message) ::XEngine::LogInfo(message)
#define XENGINE_LOG_WARN(message) ::XEngine::LogWarn(message)
#define XENGINE_LOG_ERROR(message) ::XEngine::LogError(message)

namespace XEngine
{
    inline void LogInfo(const char* message) { std::cout << "[Info] " << message << std::endl; }
    inline void LogWarn(const char* message) { std::cout << "[Warn] " << message << std::endl; }
    inline void LogError(const char* message) { std::cerr << "[Error] " << message << std::endl; }
}
