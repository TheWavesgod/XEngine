#include <XEngine/Engine/Time.h>

namespace XEngine
{
    void Time::Reset()
    {
        const auto now = Clock::now();
        m_StartTime = now;
        m_PreviousTime = now;
        m_DeltaTime = 0.0f;
        m_TotalTime = 0.0f;
        m_FrameIndex = 0;
    }

    void Time::Tick()
    {
        const auto now = Clock::now();
        const std::chrono::duration<f32> deltaTime = now - m_PreviousTime;
        const std::chrono::duration<f32> totalTime = now - m_StartTime;

        m_PreviousTime = now;
        m_DeltaTime = deltaTime.count();
        m_TotalTime = totalTime.count();
        ++m_FrameIndex;
    }

    f32 Time::GetDeltaTime() const
    {
        return m_DeltaTime;
    }

    f32 Time::GetTotalTime() const
    {
        return m_TotalTime;
    }

    u64 Time::GetFrameIndex() const
    {
        return m_FrameIndex;
    }
}
