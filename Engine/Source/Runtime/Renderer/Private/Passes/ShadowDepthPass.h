#pragma once

namespace XEngine
{
    class RenderGraph;
    struct RenderFrameContext;
    struct RenderScene;
    struct RenderResourceContext;

    void AddShadowDepthPass(
        RenderGraph& graph,
        const RenderFrameContext& frame,
        const RenderScene& scene,
        RenderResourceContext& resources);
}