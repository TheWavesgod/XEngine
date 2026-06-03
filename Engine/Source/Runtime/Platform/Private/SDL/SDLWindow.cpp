#include "SDLWindow.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    SDLWindow::SDLWindow(const WindowDesc& desc)
        : m_Title(desc.Title)
        , m_Width(desc.Width)
        , m_Height(desc.Height)
    {
#if defined(XENGINE_ENABLE_SDL)
        SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(0);
        if (desc.Resizable)
        {
            flags |= SDL_WINDOW_RESIZABLE;
        }
        if (desc.Maximized)
        {
            flags |= SDL_WINDOW_MAXIMIZED;
        }

        m_Window = SDL_CreateWindow(m_Title.c_str(), static_cast<int>(m_Width), static_cast<int>(m_Height), flags);
        if (m_Window == nullptr)
        {
            std::string message = "Failed to create SDL window: ";
            message += SDL_GetError();
            XENGINE_LOG_ERROR(message);
        }

        XENGINE_ASSERT(m_Window != nullptr, "Failed to create SDL window");
#endif
    }

    SDLWindow::~SDLWindow()
    {
#if defined(XENGINE_ENABLE_SDL)
        if (m_Window != nullptr)
        {
            XENGINE_LOG_INFO("Destroying SDL window");
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
#endif
    }

    void SDLWindow::PollEvents()
    {
#if defined(XENGINE_ENABLE_SDL)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                m_ShouldClose = true;
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                m_Width = static_cast<u32>(event.window.data1);
                m_Height = static_cast<u32>(event.window.data2);

                std::string message = "SDL window resized: ";
                message += std::to_string(m_Width);
                message += "x";
                message += std::to_string(m_Height);
                XENGINE_LOG_INFO(message);
            }
        }
#endif
    }

    bool SDLWindow::ShouldClose() const
    {
        return m_ShouldClose;
    }

    u32 SDLWindow::GetWidth() const
    {
        return m_Width;
    }

    u32 SDLWindow::GetHeight() const
    {
        return m_Height;
    }

    std::string_view SDLWindow::GetTitle() const
    {
        return m_Title;
    }

    NativeWindowHandle SDLWindow::GetNativeHandle() const
    {
        NativeWindowHandle handle;
#if defined(XENGINE_ENABLE_SDL)
        handle.Window = static_cast<void*>(m_Window);
#endif
        handle.Display = nullptr;
        return handle;
    }
}
