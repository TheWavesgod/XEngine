#include "PresentPass.h"

#include "../RenderGraph/RenderGraph.h"

namespace XEngine
{
    void AddPresentPass(RenderGraph& graph)
    {
        RenderGraphPassDesc desc;
        desc.Name = "PresentPass";
        desc.Type = RenderGraphPassType::Present;

        graph.AddPass(
            desc,
            [](RenderGraphBuilder&)
            {
            },
            [](RenderGraphContext&)
            {
                // TODO: Present is currently handled by RHIDevice::EndFrame().
                // Future RHI will expose explicit present command.
            });
    }
}
