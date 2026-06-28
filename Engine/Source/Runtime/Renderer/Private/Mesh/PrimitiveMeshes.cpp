#include "PrimitiveMeshes.h"

#include <XEngine/Math/MathFunctions.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>
#include <XEngine/RHI/RHIUploadManager.h>

#include <array>

namespace XEngine
{
    StaticMesh CreateHardcodedCubeMesh(RHIDevice& device)
    {
        const std::array<LegacyMeshVertex, 8> vertices = {
            LegacyMeshVertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, Math::Normalize(Vec3 { -0.5f, -0.5f, -0.5f }), { 0.0f, 0.0f } },
            LegacyMeshVertex { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, Math::Normalize(Vec3 {  0.5f, -0.5f, -0.5f }), { 1.0f, 0.0f } },
            LegacyMeshVertex { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, Math::Normalize(Vec3 {  0.5f,  0.5f, -0.5f }), { 1.0f, 1.0f } },
            LegacyMeshVertex { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, Math::Normalize(Vec3 { -0.5f,  0.5f, -0.5f }), { 0.0f, 1.0f } },
            LegacyMeshVertex { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f }, Math::Normalize(Vec3 { -0.5f, -0.5f,  0.5f }), { 0.0f, 0.0f } },
            LegacyMeshVertex { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f }, Math::Normalize(Vec3 {  0.5f, -0.5f,  0.5f }), { 1.0f, 0.0f } },
            LegacyMeshVertex { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f }, Math::Normalize(Vec3 {  0.5f,  0.5f,  0.5f }), { 1.0f, 1.0f } },
            LegacyMeshVertex { { -0.5f,  0.5f,  0.5f }, { 0.2f, 0.6f, 1.0f }, Math::Normalize(Vec3 { -0.5f,  0.5f,  0.5f }), { 0.0f, 1.0f } }
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
        RHIResourceFactory& factory = device.GetResourceFactory();

        RHIBufferDesc vertexDesc;
        vertexDesc.Size = sizeof(LegacyMeshVertex) * vertices.size();
        vertexDesc.Usage = RHIBufferUsage::Vertex;
        vertexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        vertexDesc.DebugName = "Cube vertices";
        mesh.VertexBuffer = factory.CreateBuffer(vertexDesc);
        if (mesh.VertexBuffer)
        {
            device.GetUploadManager().UploadBuffer(
                *mesh.VertexBuffer, vertices.data(), vertexDesc.Size);
        }

        RHIBufferDesc indexDesc;
        indexDesc.Size = sizeof(u32) * indices.size();
        indexDesc.Usage = RHIBufferUsage::Index;
        indexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        indexDesc.DebugName = "Cube indices";
        mesh.IndexBuffer = factory.CreateBuffer(indexDesc);
        if (mesh.IndexBuffer)
        {
            device.GetUploadManager().UploadBuffer(
                *mesh.IndexBuffer, indices.data(), indexDesc.Size);
        }

        return mesh;
    }
}
