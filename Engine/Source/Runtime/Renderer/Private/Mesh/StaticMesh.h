#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Vector.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>

#include <memory>

namespace XEngine
{
    struct MeshVertex
    {
        Vector3 Position;
        Vector3 Color;
    };

    class StaticMesh
    {
    public:
        std::shared_ptr<RHIBuffer> VertexBuffer;
        std::shared_ptr<RHIBuffer> IndexBuffer;
        u32 VertexCount = 0;
        u32 IndexCount = 0;
        RHIIndexFormat IndexFormat = RHIIndexFormat::UInt32;
    };
}
