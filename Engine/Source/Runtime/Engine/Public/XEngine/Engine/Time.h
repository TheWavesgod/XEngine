#pragma once

#include <XEngine/Core/Types.h>

#include <chrono>

namespace XEngine
{
    class Time
    {
    public:
        void Reset();
        void Tick();

        f32 GetDeltaTime() const;
        f32 GetTotalTime() const;
        u64 GetFrameIndex() const;

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_StartTime {};
        Clock::time_point m_PreviousTime {};

        f32 m_DeltaTime = 0.0f;
        f32 m_TotalTime = 0.0f;
        u64 m_FrameIndex = 0;
    };
}
