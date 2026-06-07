#include "SDLWindow.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    namespace
    {
#if defined(XENGINE_ENABLE_SDL)
        KeyCode ToKeyCode(SDL_Scancode scancode)
        {
            switch (scancode)
            {
            case SDL_SCANCODE_W:
                return KeyCode::W;
            case SDL_SCANCODE_A:
                return KeyCode::A;
            case SDL_SCANCODE_S:
                return KeyCode::S;
            case SDL_SCANCODE_D:
                return KeyCode::D;
            case SDL_SCANCODE_Q:
                return KeyCode::Q;
            case SDL_SCANCODE_E:
                return KeyCode::E;
            case SDL_SCANCODE_LSHIFT:
                return KeyCode::LeftShift;
            case SDL_SCANCODE_RSHIFT:
                return KeyCode::RightShift;
            case SDL_SCANCODE_ESCAPE:
                return KeyCode::Escape;
            default:
                return KeyCode::Unknown;
            }
        }

        MouseButton ToMouseButton(Uint8 button)
        {
            switch (button)
            {
            case SDL_BUTTON_LEFT:
                return MouseButton::Left;
            case SDL_BUTTON_RIGHT:
                return MouseButton::Right;
            case SDL_BUTTON_MIDDLE:
                return MouseButton::Middle;
            default:
                return MouseButton::Left;
            }
        }
#endif
    }

    SDLWindow::SDLWindow(const WindowDesc& desc)
        : m_Title(desc.Title)
        , m_Width(desc.Width)
        , m_Height(desc.Height)
    {
#if defined(XENGINE_ENABLE_SDL)
        SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
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

    void SDLWindow::PollEvents(std::vector<PlatformEvent>& events)
    {
#if defined(XENGINE_ENABLE_SDL)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                m_ShouldClose = true;

                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::WindowClose;
                events.push_back(platformEvent);
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

                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::WindowResize;
                platformEvent.Width = m_Width;
                platformEvent.Height = m_Height;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::WindowMinimized;
                platformEvent.Width = m_Width;
                platformEvent.Height = m_Height;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_WINDOW_RESTORED)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::WindowRestored;
                platformEvent.Width = m_Width;
                platformEvent.Height = m_Height;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = event.type == SDL_EVENT_KEY_DOWN ?
                    PlatformEventType::KeyDown :
                    PlatformEventType::KeyUp;
                platformEvent.Key = ToKeyCode(event.key.scancode);
                platformEvent.Repeat = event.key.repeat;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ?
                    PlatformEventType::MouseButtonDown :
                    PlatformEventType::MouseButtonUp;
                platformEvent.Button = ToMouseButton(event.button.button);
                platformEvent.MouseX = event.button.x;
                platformEvent.MouseY = event.button.y;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::MouseMove;
                platformEvent.MouseX = event.motion.x;
                platformEvent.MouseY = event.motion.y;
                platformEvent.MouseDeltaX = event.motion.xrel;
                platformEvent.MouseDeltaY = event.motion.yrel;
                events.push_back(platformEvent);
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                PlatformEvent platformEvent;
                platformEvent.Type = PlatformEventType::MouseWheel;
                platformEvent.WheelDeltaX = event.wheel.x;
                platformEvent.WheelDeltaY = event.wheel.y;
                events.push_back(platformEvent);
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
