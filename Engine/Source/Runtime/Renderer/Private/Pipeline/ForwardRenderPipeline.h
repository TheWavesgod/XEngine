#pragma once

#include "RenderPipeline.h"
#include "../RenderGraph/RenderGraph.h"

namespace XEngine
{
    class ForwardRenderPipeline final : public RenderPipeline
    {
    public:
        bool Initialize(RenderResourceContext& resources) override;
        void Shutdown() override;

        void Render(
            const RenderFrameContext& frame,
            const RenderScene& scene,
            RenderResourceContext& resources) override;

    private:
        RenderGraph m_Graph;
        bool m_Initialized = false;
    };
}
