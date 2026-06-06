#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Engine/Subsystem.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHITexture.h>

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
        void CreateDefaultTextureValidationResources();

        Engine* m_Engine = nullptr;
        std::unique_ptr<RHIDevice> m_Device;
        std::shared_ptr<RHITexture> m_DefaultWhiteTexture;
        std::shared_ptr<RHITexture> m_DefaultNormalTexture;
        std::shared_ptr<RHISampler> m_DefaultLinearRepeatSampler;
        bool m_PendingResize = false;
        u32 m_PendingResizeWidth = 0;
        u32 m_PendingResizeHeight = 0;
        bool m_Initialized = false;
    };
}
