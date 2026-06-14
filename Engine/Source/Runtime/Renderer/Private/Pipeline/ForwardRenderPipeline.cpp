#include "ForwardRenderPipeline.h"

#include "RenderFrameContext.h"
#include "../Passes/ClearPass.h"
#include "../Passes/ForwardOpaquePass.h"
#include "../Passes/PresentPass.h"
#include "../RenderGraph/RenderGraphContext.h"
#include "../Resources/RenderResourceContext.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    bool ForwardRenderPipeline::Initialize(RenderResourceContext& resources)
    {
        if (!resources.IsValid())
        {
            XENGINE_LOG_ERROR("ForwardRenderPipeline requires a valid RenderResourceContext");
            return false;
        }

        m_Initialized = true;
        XENGINE_LOG_INFO("ForwardRenderPipeline initialized");
        return true;
    }

    void ForwardRenderPipeline::Shutdown()
    {
        if (m_Initialized)
        {
            XENGINE_LOG_INFO("ForwardRenderPipeline shutdown");
        }
        m_Graph.Clear();
        m_Initialized = false;
    }

    void ForwardRenderPipeline::Render(
        const RenderFrameContext& frame,
        const RenderScene& scene,
        RenderResourceContext& resources)
    {
        if (!m_Initialized || frame.Device == nullptr || frame.CommandList == nullptr ||
            !resources.IsValid())
        {
            return;
        }

        m_Graph.Clear();

        RHIColor clearColor;
        clearColor.R = 0.1f;
        clearColor.G = 0.1f;
        clearColor.B = 0.15f;
        clearColor.A = 1.0f;

        AddClearPass(m_Graph, clearColor);
        AddForwardOpaquePass(m_Graph, frame, scene, resources);
        // TODO Stage 8B/8C/8D: add lighting and shadow passes to this same graph.
        AddPresentPass(m_Graph);

        m_Graph.Compile();
        RenderGraphContext graphContext(*frame.Device, frame.CommandList);
        m_Graph.Execute(graphContext);
    }
}
