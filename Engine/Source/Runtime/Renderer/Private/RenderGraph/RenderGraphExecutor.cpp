#include "RenderGraphExecutor.h"

#include "RenderGraph.h"
#include "RenderGraphContext.h"

namespace XEngine
{
    void RenderGraphExecutor::Execute(RenderGraph& graph, RenderGraphContext& context)
    {
        graph.Execute(context);
    }
}
