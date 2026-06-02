#pragma once

#if defined(_WIN32)
    #define XENGINE_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define XENGINE_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define XENGINE_PLATFORM_LINUX 1
#else
    #define XENGINE_PLATFORM_UNKNOWN 1
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
    #define XENGINE_DEBUG 1
#else
    #define XENGINE_RELEASE 1
#endif

#if defined(XENGINE_DEBUG)
    #define XENGINE_ENABLE_ASSERTS 1
#endif
