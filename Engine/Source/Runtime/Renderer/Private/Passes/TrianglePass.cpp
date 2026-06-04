#include "TrianglePass.h"

#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHICommandList.h>

namespace XEngine
{
    void AddTrianglePass(RenderGraph& graph, RHIPipeline* pipeline)
    {
        RenderGraphPassDesc desc;
        desc.Name = "TrianglePass";
        desc.Type = RenderGraphPassType::Graphics;

        graph.AddPass(
            desc,
            [](RenderGraphBuilder&)
            {
                // TODO Stage 5: declare swapchain color attachment write access.
            },
            [pipeline](RenderGraphContext& context)
            {
                RHICommandList* commandList = context.GetCommandList();
                if (commandList == nullptr || pipeline == nullptr)
                {
                    return;
                }

                commandList->SetGraphicsPipeline(pipeline);
                commandList->Draw(3, 1, 0, 0);
            });
    }
}
