#pragma once

#include <XEngine/Core/Defines.h>

namespace XEngine
{
    void DebugBreak();
    void ReportAssertionFailure(const char* expression, const char* file, int line, const char* message);
}

#if defined(XENGINE_ENABLE_ASSERTS)
    #define XENGINE_ASSERT(expression, message)                                                         \
        do                                                                                              \
        {                                                                                               \
            if (!(expression))                                                                          \
            {                                                                                           \
                ::XEngine::ReportAssertionFailure(#expression, __FILE__, __LINE__, message);            \
                ::XEngine::DebugBreak();                                                                \
            }                                                                                           \
        } while (false)
#else
    #define XENGINE_ASSERT(expression, message) ((void)0)
#endif
