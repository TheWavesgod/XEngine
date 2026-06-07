#include "RenderMeshManager.h"

#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>

#include <string>
#include <utility>

namespace XEngine
{
    namespace
    {
        u64 MakeAssetMeshCacheKey(AssetHandle handle)
        {
            return (static_cast<u64>(handle.Generation) << 32u) | static_cast<u64>(handle.Index);
        }
    }

    void RenderMeshManager::Initialize(RHIDevice* device)
    {
        if (m_Initialized)
        {
            return;
        }

        XENGINE_ASSERT(device != nullptr, "RenderMeshManager requires RHIDevice");
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderMeshManager requires a valid RHIDevice");
            return;
        }

        m_Device = device;
        m_Initialized = true;
        XENGINE_LOG_INFO("RenderMeshManager initialized");
    }

    void RenderMeshManager::Shutdown()
    {
        if (!m_Initialized && m_Meshes.empty())
        {
            return;
        }

        XENGINE_LOG_INFO("RenderMeshManager shutdown");
        m_AssetMeshCache.clear();
        m_Meshes.clear();
        m_Device = nullptr;
        m_Initialized = false;
    }

    MeshHandle RenderMeshManager::CreateMeshFromAsset(const MeshAsset& asset)
    {
        if (m_Device == nullptr || !m_Device->IsValid())
        {
            XENGINE_LOG_ERROR("Cannot create RenderMesh without a valid RHIDevice");
            return {};
        }

        if (!asset.IsValid())
        {
            XENGINE_LOG_WARN("MeshAsset is invalid");
            return {};
        }

        RHIBufferDesc vertexDesc;
        vertexDesc.Size = sizeof(MeshVertex) * asset.Vertices.size();
        vertexDesc.Usage = RHIBufferUsage::Vertex | RHIBufferUsage::TransferDst;
        vertexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        vertexDesc.DebugName = asset.Name.c_str();

        std::shared_ptr<RHIBuffer> vertexBuffer =
            m_Device->CreateBuffer(vertexDesc, asset.Vertices.data(), vertexDesc.Size);
        if (!vertexBuffer)
        {
            XENGINE_LOG_ERROR(std::string("Failed to create vertex buffer for mesh: ") + asset.Name);
            return {};
        }

        RHIBufferDesc indexDesc;
        indexDesc.Size = sizeof(u32) * asset.Indices.size();
        indexDesc.Usage = RHIBufferUsage::Index | RHIBufferUsage::TransferDst;
        indexDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
        indexDesc.DebugName = asset.Name.c_str();

        std::shared_ptr<RHIBuffer> indexBuffer =
            m_Device->CreateBuffer(indexDesc, asset.Indices.data(), indexDesc.Size);
        if (!indexBuffer)
        {
            XENGINE_LOG_ERROR(std::string("Failed to create index buffer for mesh: ") + asset.Name);
            return {};
        }

        RenderMesh mesh;
        mesh.Name = asset.Name;
        mesh.VertexBuffer = std::move(vertexBuffer);
        mesh.IndexBuffer = std::move(indexBuffer);
        mesh.VertexCount = static_cast<u32>(asset.Vertices.size());
        mesh.IndexCount = static_cast<u32>(asset.Indices.size());
        mesh.IndexFormat = RHIIndexFormat::UInt32;

        mesh.Submeshes.reserve(asset.Submeshes.size());
        for (const MeshSubmesh& submesh : asset.Submeshes)
        {
            mesh.Submeshes.push_back(RenderSubmesh {
                submesh.FirstIndex,
                submesh.IndexCount,
                static_cast<i32>(submesh.VertexOffset),
                submesh.MaterialSlot
            });
        }

        return AddRenderMesh(std::move(mesh));
    }

    MeshHandle RenderMeshManager::GetOrCreateMeshFromAsset(AssetHandle assetHandle, const MeshAsset& asset)
    {
        if (!assetHandle.IsValid())
        {
            return CreateMeshFromAsset(asset);
        }

        const u64 key = MakeAssetMeshCacheKey(assetHandle);
        const auto cached = m_AssetMeshCache.find(key);
        if (cached != m_AssetMeshCache.end() && IsValid(cached->second))
        {
            return cached->second;
        }

        MeshHandle handle = CreateMeshFromAsset(asset);
        if (IsValid(handle))
        {
            m_AssetMeshCache[key] = handle;
        }

        return handle;
    }

    RenderMesh* RenderMeshManager::GetMesh(MeshHandle handle)
    {
        return const_cast<RenderMesh*>(static_cast<const RenderMeshManager*>(this)->GetMesh(handle));
    }

    const RenderMesh* RenderMeshManager::GetMesh(MeshHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &m_Meshes[handle.Index];
    }

    bool RenderMeshManager::IsValid(MeshHandle handle) const
    {
        if (!handle.IsValid() || handle.Index >= m_Meshes.size())
        {
            return false;
        }

        return m_Meshes[handle.Index].Generation == handle.Generation;
    }

    MeshHandle RenderMeshManager::AddRenderMesh(RenderMesh mesh)
    {
        mesh.Generation = 1;

        MeshHandle handle;
        handle.Index = static_cast<u32>(m_Meshes.size());
        handle.Generation = mesh.Generation;

        m_Meshes.push_back(std::move(mesh));
        return handle;
    }
}
