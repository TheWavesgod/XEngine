#pragma once

#include "ForwardMeshPass.h"

#include <vector>

namespace XEngine
{
    class RenderGraph;
    class RHIPipeline;
    class MaterialSystem;

    void AddForwardOpaquePass(
        RenderGraph& graph,
        RHIPipeline* pbrPipeline,
        MaterialSystem* materialSystem,
        const std::vector<RenderObject>& objects);
}
