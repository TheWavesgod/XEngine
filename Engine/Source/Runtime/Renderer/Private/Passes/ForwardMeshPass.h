#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Matrix.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/Renderer/Mesh.h>
#include <XEngine/Renderer/RenderScene.h>

#include <vector>

namespace XEngine
{
    class RenderGraph;
    class RHIPipeline;
    class MaterialSystem;
    class RenderMeshManager;

    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
        MaterialSystem* materialSystem,
        RenderMeshManager* meshManager,
        const std::vector<RenderObject>& objects);
}
