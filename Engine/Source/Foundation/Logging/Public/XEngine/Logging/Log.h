#pragma once

#include <string_view>

namespace XEngine
{
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    class Log
    {
    public:
        static void Initialize();
        static void Shutdown();

        static void SetLevel(LogLevel level);

        static void Trace(std::string_view message);
        static void Debug(std::string_view message);
        static void Info(std::string_view message);
        static void Warn(std::string_view message);
        static void Error(std::string_view message);
        static void Critical(std::string_view message);
    };
}

#define XENGINE_LOG_TRACE(message) ::XEngine::Log::Trace(message)
#define XENGINE_LOG_DEBUG(message) ::XEngine::Log::Debug(message)
#define XENGINE_LOG_INFO(message) ::XEngine::Log::Info(message)
#define XENGINE_LOG_WARN(message) ::XEngine::Log::Warn(message)
#define XENGINE_LOG_ERROR(message) ::XEngine::Log::Error(message)
#define XENGINE_LOG_CRITICAL(message) ::XEngine::Log::Critical(message)
