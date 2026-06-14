#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/AABB.h>
#include <XEngine/Math/MathTypes.h>

#include <string>
#include <vector>

namespace XEngine
{
    // CPU-side vertex format for imported static mesh assets.
    // Renderer systems convert this data into GPU buffers.
    struct MeshVertex
    {
        Vec3 Position { 0.0f, 0.0f, 0.0f };
        Vec3 Normal { 0.0f, 0.0f, 1.0f };
        Vec4 Tangent { 1.0f, 0.0f, 0.0f, 1.0f };
        Vec2 TexCoord0 { 0.0f, 0.0f };
    };

    // CPU-side submesh range. MaterialSlot is a Stage 7D placeholder.
    struct MeshSubmesh
    {
        u32 FirstIndex = 0;
        u32 IndexCount = 0;
        u32 VertexOffset = 0;
        u32 MaterialSlot = 0;
        AABB Bounds {};
    };

    // CPU-side mesh asset owned by AssetSystem. It contains no RHI or backend objects.
    struct MeshAsset
    {
        std::string Name;
        std::string SourcePath;

        std::vector<MeshVertex> Vertices;
        std::vector<u32> Indices;
        std::vector<MeshSubmesh> Submeshes;

        AABB Bounds {};

        bool IsValid() const
        {
            return !Vertices.empty() && !Indices.empty() && !Submeshes.empty();
        }
    };
}
