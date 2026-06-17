#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Renderer/RendererDebugSettings.h>
#include <XEngine/Renderer/RenderView.h>
#include <XEngine/RHI/RHITypes.h>

#include <functional>
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

        void SetOverlayCallback(std::function<void()> callback);
        void SetViewProvider(std::function<bool(RenderView&)> provider);
        void SetOutputProvider(std::function<bool(RHIRenderOutputDesc&)> provider);
        RendererDebugSettings& GetDebugSettings();
        const RendererDebugSettings& GetDebugSettings() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
