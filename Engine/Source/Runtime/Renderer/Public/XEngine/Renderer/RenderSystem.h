#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>

namespace XEngine
{
    class RenderSystem final : public ISubsystem
    {
    public:
        RenderSystem();
        ~RenderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
