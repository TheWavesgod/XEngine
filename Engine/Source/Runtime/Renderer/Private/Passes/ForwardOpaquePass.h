#pragma once

#include <XEngine/Math/Matrix.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/RenderScene.h>

namespace XEngine
{
    class RenderGraph;

    struct RenderFrameContext;
    struct RenderResourceContext;


    void AddForwardOpaquePass(
        RenderGraph& graph,
        const RenderFrameContext& frameContext,
        const RenderScene& renderScene,
        RenderResourceContext& resources);
}
