#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/Renderer/Mesh.h>
#include <XEngine/Renderer/RenderScene.h>

#include <vector>

namespace XEngine
{
    class RenderGraph;
    class RHIPipeline;
    class RenderMaterialSystem;
    class RenderMeshManager;

    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
        RenderMaterialSystem* materialSystem,
        RenderMeshManager* meshManager,
        const std::vector<RenderObject>& objects);
}
