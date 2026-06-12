#pragma once

namespace XEngine
{
    struct RenderFrameContext;
    struct RenderResourceContext;
    struct RenderScene;

    class RenderPipeline
    {
    public:
        virtual ~RenderPipeline() = default;

        virtual bool Initialize(RenderResourceContext& resources) = 0;
        virtual void Shutdown() = 0;
        
        virtual void Render(const RenderFrameContext& frameContext,
            const RenderScene& scene,
            RenderResourceContext& resources) = 0;
    };
} // namespace XEngine
