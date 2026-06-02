#include <XEngine/Core/Assert.h>

#include <XEngine/Logging/Log.h>

#include <cstdlib>
#include <string>

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

namespace XEngine
{
    void DebugBreak()
    {
#if defined(_MSC_VER)
        __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
#else
        std::abort();
#endif
    }

    void ReportAssertionFailure(const char* expression, const char* file, int line, const char* message)
    {
        std::string logMessage = "Assertion failed: ";
        logMessage += expression;
        logMessage += " | ";
        logMessage += file;
        logMessage += ":";
        logMessage += std::to_string(line);
        logMessage += " | ";
        logMessage += message;

        XENGINE_LOG_CRITICAL(logMessage);
    }
}
