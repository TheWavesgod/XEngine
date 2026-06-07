#pragma once

#include <XEngine/Math/Matrix.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/RenderScene.h>

namespace XEngine
{
    class RenderGraph;
    class RHIPipeline;
    class MaterialSystem;
    class RenderMeshManager;

    void AddForwardOpaquePass(
        RenderGraph& graph,
        RHIPipeline* pbrPipeline,
        MaterialSystem* materialSystem,
        RenderMeshManager* meshManager,
        const RenderScene& renderScene,
        const Mat4& viewProjection);
}
