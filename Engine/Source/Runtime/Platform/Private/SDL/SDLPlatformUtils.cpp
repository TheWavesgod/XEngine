#include "SDLPlatformUtils.h"

#include <XEngine/Logging/Log.h>

#if defined(XENGINE_ENABLE_SDL)
    #include <SDL3/SDL.h>
#endif

#include <string>

namespace XEngine::SDLPlatformUtils
{
    bool InitializeVideo()
    {
#if defined(XENGINE_ENABLE_SDL)
        if (SDL_Init(SDL_INIT_VIDEO))
        {
            return true;
        }

        std::string message = "SDL video initialization failed: ";
        message += SDL_GetError();
        XENGINE_LOG_ERROR(message);
        return false;
#else
        return false;
#endif
    }

    void QuitVideo()
    {
#if defined(XENGINE_ENABLE_SDL)
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
#endif
    }
}
