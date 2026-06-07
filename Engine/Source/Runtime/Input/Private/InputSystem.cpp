#include <XEngine/Input/InputSystem.h>

#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/PlatformEvents.h>
#include <XEngine/Platform/PlatformSystem.h>

namespace XEngine
{
    InputSystem::InputSystem() = default;

    InputSystem::~InputSystem()
    {
        OnDestroy();
    }

    void InputSystem::OnCreate(const SubsystemContext& context)
    {
        if (m_Initialized)
        {
            return;
        }

        if (context.Engine != nullptr)
        {
            m_PlatformSystem = context.Engine->GetSubsystemManager().GetSubsystem<PlatformSystem>();
        }

        m_Initialized = true;
        XENGINE_LOG_INFO("InputSystem initialized");
    }

    void InputSystem::OnDestroy()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_CurrentKeys = {};
        m_PreviousKeys = {};
        m_CurrentMouseButtons = {};
        m_PreviousMouseButtons = {};
        m_MousePosition = {};
        m_MouseDelta = {};
        m_MouseWheelDelta = 0.0f;
        m_PlatformSystem = nullptr;
        m_Initialized = false;
        XENGINE_LOG_INFO("InputSystem shutdown");
    }

    void InputSystem::OnUpdate(float deltaTime)
    {
        (void)deltaTime;
        BeginFrame();

        if (m_PlatformSystem != nullptr)
        {
            for (const PlatformEvent& event : m_PlatformSystem->GetEvents())
            {
                ProcessEvent(event);
            }
        }

        EndFrame();
    }

    void InputSystem::BeginFrame()
    {
        m_PreviousKeys = m_CurrentKeys;
        m_PreviousMouseButtons = m_CurrentMouseButtons;
        m_MouseDelta = {};
        m_MouseWheelDelta = 0.0f;
    }

    void InputSystem::ProcessEvent(const PlatformEvent& event)
    {
        switch (event.Type)
        {
        case PlatformEventType::KeyDown:
            if (event.Key != KeyCode::Unknown)
            {
                m_CurrentKeys[ToIndex(event.Key)] = true;
            }
            break;
        case PlatformEventType::KeyUp:
            if (event.Key != KeyCode::Unknown)
            {
                m_CurrentKeys[ToIndex(event.Key)] = false;
            }
            break;
        case PlatformEventType::MouseButtonDown:
            m_CurrentMouseButtons[ToIndex(event.Button)] = true;
            break;
        case PlatformEventType::MouseButtonUp:
            m_CurrentMouseButtons[ToIndex(event.Button)] = false;
            break;
        case PlatformEventType::MouseMove:
            m_MousePosition = Vec2 { event.MouseX, event.MouseY };
            m_MouseDelta += Vec2 { event.MouseDeltaX, event.MouseDeltaY };
            break;
        case PlatformEventType::MouseWheel:
            m_MouseWheelDelta += event.WheelDeltaY;
            break;
        default:
            break;
        }
    }

    void InputSystem::EndFrame() {}

    bool InputSystem::IsKeyDown(KeyCode key) const
    {
        return key != KeyCode::Unknown && m_CurrentKeys[ToIndex(key)];
    }

    bool InputSystem::WasKeyPressed(KeyCode key) const
    {
        return key != KeyCode::Unknown && m_CurrentKeys[ToIndex(key)] && !m_PreviousKeys[ToIndex(key)];
    }

    bool InputSystem::WasKeyReleased(KeyCode key) const
    {
        return key != KeyCode::Unknown && !m_CurrentKeys[ToIndex(key)] && m_PreviousKeys[ToIndex(key)];
    }

    bool InputSystem::IsMouseButtonDown(MouseButton button) const
    {
        return m_CurrentMouseButtons[ToIndex(button)];
    }

    bool InputSystem::WasMouseButtonPressed(MouseButton button) const
    {
        return m_CurrentMouseButtons[ToIndex(button)] && !m_PreviousMouseButtons[ToIndex(button)];
    }

    bool InputSystem::WasMouseButtonReleased(MouseButton button) const
    {
        return !m_CurrentMouseButtons[ToIndex(button)] && m_PreviousMouseButtons[ToIndex(button)];
    }

    Vec2 InputSystem::GetMousePosition() const
    {
        return m_MousePosition;
    }

    Vec2 InputSystem::GetMouseDelta() const
    {
        return m_MouseDelta;
    }

    float InputSystem::GetMouseWheelDelta() const
    {
        return m_MouseWheelDelta;
    }

    std::size_t InputSystem::ToIndex(KeyCode key)
    {
        return static_cast<std::size_t>(key);
    }

    std::size_t InputSystem::ToIndex(MouseButton button)
    {
        return static_cast<std::size_t>(button);
    }
}
