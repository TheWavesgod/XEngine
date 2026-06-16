#pragma once

#include <XEngine/Platform/Window.h>

#if defined(XENGINE_ENABLE_SDL)
    #include <SDL3/SDL.h>
#endif

#include <string>
#include <string_view>

namespace XEngine
{
    class SDLWindow final : public Window
    {
    public:
        explicit SDLWindow(const WindowDesc& desc);
        ~SDLWindow() override;

        void PollEvents(std::vector<PlatformEvent>& events) override;

        bool ShouldClose() const override;

        u32 GetWidth() const override;
        u32 GetHeight() const override;
        void SetCursorVisible(bool visible) override;
        void SetRelativeMouseMode(bool enabled) override;
        bool IsFocused() const override;

        std::string_view GetTitle() const override;

        NativeWindowHandle GetNativeHandle() const override;

    private:
#if defined(XENGINE_ENABLE_SDL)
        SDL_Window* m_Window = nullptr;
#endif

        std::string m_Title;
        u32 m_Width = 0;
        u32 m_Height = 0;
        bool m_ShouldClose = false;
        bool m_Focused = true;
    };
}
