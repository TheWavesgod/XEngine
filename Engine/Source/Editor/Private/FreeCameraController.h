#pragma once

#include <XEngine/Input/InputTypes.h>
#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    class EditorCamera;
    struct PlatformEvent;

    class FreeCameraController
    {
    public:
        void ProcessEvent(const PlatformEvent& event);
        void Update(EditorCamera& camera, float deltaTime, bool active);
        void ResetTransientInput();

    private:
        bool m_Forward = false;
        bool m_Backward = false;
        bool m_Left = false;
        bool m_Right = false;
        bool m_Down = false;
        bool m_Up = false;
        bool m_Fast = false;
        Vec2 m_MouseDelta { 0.0f, 0.0f };
        float m_MoveSpeed = 4.0f;
        float m_FastMultiplier = 4.0f;
        float m_MouseSensitivity = 0.12f;
    };
}
