#pragma once

namespace XEngine
{
    class RenderGraph;
    class RenderGraphContext;

    class RenderGraphExecutor
    {
    public:
        void Execute(RenderGraph& graph, RenderGraphContext& context);
    };
}
