#include "PrimitiveMeshes.h"

#include <XEngine/RHI/RHIDevice.h>

#include <array>

namespace XEngine
{
    StaticMesh CreateHardcodedCubeMesh(RHIDevice& device)
    {
        const std::array<MeshVertex, 8> vertices = {
            MeshVertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
            MeshVertex { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
            MeshVertex { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
            MeshVertex { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
            MeshVertex { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
            MeshVertex { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
            MeshVertex { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f } },
            MeshVertex { { -0.5f,  0.5f,  0.5f }, { 0.2f, 0.6f, 1.0f } }
        };

        const std::array<u32, 36> indices = {
            4, 5, 6, 4, 6, 7,
            1, 0, 3, 1, 3, 2,
            0, 4, 7, 0, 7, 3,
            5, 1, 2, 5, 2, 6,
            3, 7, 6, 3, 6, 2,
            0, 1, 5, 0, 5, 4
        };

        StaticMesh mesh;
        mesh.VertexCount = static_cast<u32>(vertices.size());
        mesh.IndexCount = static_cast<u32>(indices.size());
        mesh.IndexFormat = RHIIndexFormat::UInt32;

        RHIBufferDesc vertexDesc;
        vertexDesc.Size = sizeof(MeshVertex) * vertices.size();
        vertexDesc.Usage = RHIBufferUsage::Vertex;
        vertexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        vertexDesc.DebugName = "Cube vertices";
        mesh.VertexBuffer = device.CreateBuffer(vertexDesc, vertices.data(), vertexDesc.Size);

        RHIBufferDesc indexDesc;
        indexDesc.Size = sizeof(u32) * indices.size();
        indexDesc.Usage = RHIBufferUsage::Index;
        indexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        indexDesc.DebugName = "Cube indices";
        mesh.IndexBuffer = device.CreateBuffer(indexDesc, indices.data(), indexDesc.Size);

        return mesh;
    }
}
