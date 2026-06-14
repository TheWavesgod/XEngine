#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>

#include <memory>

namespace XEngine
{
    struct LegacyMeshVertex
    {
        Vec3 Position;
        Vec3 Color;
        Vec3 Normal;
        Vec2 UV;
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
