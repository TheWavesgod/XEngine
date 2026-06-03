#include "ClearPass.h"

#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    void AddClearPass(RenderGraph& graph, const RHIColor& clearColor)
    {
        RenderGraphPassDesc desc;
        desc.Name = "ClearPass";
        desc.Type = RenderGraphPassType::Graphics;

        graph.AddPass(
            desc,
            [](RenderGraphBuilder&)
            {
                // TODO Stage 4+: declare swapchain write access.
            },
            [clearColor](RenderGraphContext& context)
            {
                context.GetDevice().ClearSwapchain(clearColor);
            });
    }
}
