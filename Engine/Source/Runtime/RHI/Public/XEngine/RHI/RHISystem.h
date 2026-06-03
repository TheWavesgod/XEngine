#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>

namespace XEngine
{
    class RHIDevice;

    class RHISystem final : public ISubsystem
    {
    public:
        RHISystem();
        ~RHISystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

        RHIDevice* GetDevice();
        const RHIDevice* GetDevice() const;

    private:
        std::unique_ptr<RHIDevice> m_Device;
        bool m_Initialized = false;
    };
}
