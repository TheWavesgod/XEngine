#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/AABB.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/Renderer/Mesh.h>

#include <vector>

namespace XEngine
{
    // Renderer-facing object generated from Scene components.
    // This is not an ECS component and should not be stored in Scene.
    struct RenderObject
    {
        Mat4 WorldMatrix { 1.0f };
        Mat4 PreviousWorldMatrix { 1.0f };

        MeshHandle Mesh;
        MaterialHandle Material;

        AABB WorldBounds {};

        u32 ObjectId = 0;
        u32 Flags = 0;
    };

    // Renderer-facing scene data consumed by render passes.
    // Stage 7F keeps only opaque objects; lights and transparent queues come later.
    struct RenderScene
    {
        std::vector<RenderObject> OpaqueObjects;

        void Clear()
        {
            OpaqueObjects.clear();
        }
    };
}
