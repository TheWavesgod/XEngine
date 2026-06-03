#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>

namespace XEngine
{
    class Window;

    class PlatformSystem final : public ISubsystem
    {
    public:
        PlatformSystem();
        ~PlatformSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnBeginFrame() override;

        Window* GetMainWindow();
        const Window* GetMainWindow() const;

    private:
        Engine* m_Engine = nullptr;
        std::unique_ptr<Window> m_MainWindow;
        bool m_Initialized = false;
    };
}
