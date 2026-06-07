#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Input/InputTypes.h>

#include <array>

namespace XEngine
{
    class PlatformSystem;
    struct PlatformEvent;

    // Runtime subsystem that stores per-frame keyboard and mouse state.
    // Stage 7G does not implement action mappings, rebinding, or editor focus.
    class InputSystem final : public ISubsystem
    {
    public:
        InputSystem();
        ~InputSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

        void BeginFrame();
        void ProcessEvent(const PlatformEvent& event);
        void EndFrame();

        bool IsKeyDown(KeyCode key) const;
        bool WasKeyPressed(KeyCode key) const;
        bool WasKeyReleased(KeyCode key) const;

        bool IsMouseButtonDown(MouseButton button) const;
        bool WasMouseButtonPressed(MouseButton button) const;
        bool WasMouseButtonReleased(MouseButton button) const;

        Vec2 GetMousePosition() const;
        Vec2 GetMouseDelta() const;
        float GetMouseWheelDelta() const;

    private:
        static std::size_t ToIndex(KeyCode key);
        static std::size_t ToIndex(MouseButton button);

        PlatformSystem* m_PlatformSystem = nullptr;
        std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_CurrentKeys {};
        std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_PreviousKeys {};
        std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_CurrentMouseButtons {};
        std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_PreviousMouseButtons {};

        Vec2 m_MousePosition { 0.0f, 0.0f };
        Vec2 m_MouseDelta { 0.0f, 0.0f };
        float m_MouseWheelDelta = 0.0f;
        bool m_Initialized = false;
    };
}
