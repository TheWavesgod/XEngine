#include "ForwardPipeline.h"

#include "RenderGraph/RenderGraph.h"

#include "Passes/ClearPass.h"
#include "Passes/ForwardOpaquePass.h"
#include "Passes/PresentPass.h"

void XEngine::ForwardPipeline::Render(
    const RenderFrameContext& frameContext, 
    const RenderScene& scene,
    RenderResourceContext& resources)
{
    m_RenderGraph->Clear();

    RHIColor clearColor;
    clearColor.R = 0.1f;
    clearColor.G = 0.1f;
    clearColor.B = 0.15f;
    clearColor.A = 1.0f;

    AddClearPass(*m_RenderGraph, clearColor);
    //AddForwardOpaquePass()
    //AddPresentPass();
}