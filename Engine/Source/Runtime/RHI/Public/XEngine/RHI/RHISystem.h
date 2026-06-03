#pragma once

#include <XEngine/Core/Types.h>
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
        Engine* m_Engine = nullptr;
        std::unique_ptr<RHIDevice> m_Device;
        bool m_PendingResize = false;
        u32 m_PendingResizeWidth = 0;
        u32 m_PendingResizeHeight = 0;
        bool m_Initialized = false;
    };
}
