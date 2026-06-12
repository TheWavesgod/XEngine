#pragma once

#include "RenderPipeline.h"

#include <memory>

namespace XEngine
{
    class RenderGraph;

    class ForwardPipeline final : public RenderPipeline
    {
    public:
        ForwardPipeline();
        ~ForwardPipeline() override;

        bool Initialize(RenderResourceContext& resources) override;
        void Shutdown() override;

        void Render(const RenderFrameContext& frameContext,
            const RenderScene& scene,
            RenderResourceContext& resources) override;

    private:
        std::shared_ptr<RenderGraph> m_RenderGraph;
        bool m_Initialized = false;
    };
} // namespace XEngine
