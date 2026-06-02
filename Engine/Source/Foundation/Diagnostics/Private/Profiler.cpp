#include <XEngine/Diagnostics/Profiler.h>

#include <XEngine/Diagnostics/ScopedTimer.h>

namespace XEngine
{
    ScopedTimer::ScopedTimer(std::string_view name)
        : m_Name(name)
        , m_StartTime(Clock::now())
    {
    }

    ScopedTimer::~ScopedTimer()
    {
        // TODO: Route timing data to the profiler once diagnostics are implemented.
        (void)m_Name;
        (void)m_StartTime;
    }
}
