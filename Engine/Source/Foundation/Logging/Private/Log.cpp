#include <XEngine/Logging/Log.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace
{
    std::shared_ptr<spdlog::logger> s_Logger;

    spdlog::level::level_enum ToSpdlogLevel(XEngine::LogLevel level)
    {
        switch (level)
        {
        case XEngine::LogLevel::Trace:
            return spdlog::level::trace;
        case XEngine::LogLevel::Debug:
            return spdlog::level::debug;
        case XEngine::LogLevel::Info:
            return spdlog::level::info;
        case XEngine::LogLevel::Warn:
            return spdlog::level::warn;
        case XEngine::LogLevel::Error:
            return spdlog::level::err;
        case XEngine::LogLevel::Critical:
            return spdlog::level::critical;
        case XEngine::LogLevel::Off:
            return spdlog::level::off;
        }

        return spdlog::level::info;
    }
}

namespace XEngine
{
    void Log::Initialize()
    {
        if (s_Logger)
        {
            return;
        }

        s_Logger = spdlog::stdout_color_mt("XEngine");
        s_Logger->set_pattern("[%T] [%n] [%^%l%$] %v");
        s_Logger->set_level(spdlog::level::trace);
        s_Logger->flush_on(spdlog::level::warn);

        Info("Log initialized");
    }

    void Log::Shutdown()
    {
        if (s_Logger)
        {
            Info("Log shutdown");
            s_Logger.reset();
        }

        spdlog::shutdown();
    }

    void Log::SetLevel(LogLevel level)
    {
        if (s_Logger)
        {
            s_Logger->set_level(ToSpdlogLevel(level));
        }
    }

    void Log::Trace(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->trace(message);
        }
    }

    void Log::Debug(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->debug(message);
        }
    }

    void Log::Info(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->info(message);
        }
    }

    void Log::Warn(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->warn(message);
        }
    }

    void Log::Error(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->error(message);
        }
    }

    void Log::Critical(std::string_view message)
    {
        if (s_Logger)
        {
            s_Logger->critical(message);
        }
    }
}
