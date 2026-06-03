#pragma once

#include <XEngine/Engine/Subsystem.h>

namespace XEngine
{
    class RHISystem;

    class RenderSystem final : public ISubsystem
    {
    public:
        RenderSystem();
        ~RenderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

    private:
        void Render();

        RHISystem* m_RHISystem = nullptr;
        bool m_Initialized = false;
    };
}
