#pragma once

#include <chrono>
#include <string_view>

namespace XEngine
{
    class ScopedTimer
    {
    public:
        explicit ScopedTimer(std::string_view name);
        ~ScopedTimer();

        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;

    private:
        using Clock = std::chrono::steady_clock;

        std::string_view m_Name;
        Clock::time_point m_StartTime;
    };
}
