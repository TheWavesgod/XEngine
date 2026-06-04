#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Matrix.h>

#include <vector>

namespace XEngine
{
    class RenderGraph;
    class RHIPipeline;
    class StaticMesh;

    struct RenderObject
    {
        StaticMesh* Mesh = nullptr;
        Matrix4 Model {};
        Matrix4 ModelViewProjection {};
        u32 ObjectId = 0;
        u32 MeshId = 0;
        u32 MaterialId = 0;
    };

    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
        const std::vector<RenderObject>& objects);
}
