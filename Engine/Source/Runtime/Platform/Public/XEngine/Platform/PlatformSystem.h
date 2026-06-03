#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Platform/PlatformEvents.h>

#include <memory>
#include <vector>

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

        const std::vector<PlatformEvent>& GetEvents() const;

    private:
        Engine* m_Engine = nullptr;
        std::unique_ptr<Window> m_MainWindow;
        std::vector<PlatformEvent> m_Events;
        bool m_Initialized = false;
    };
}
