#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Core/Types.h>
#include <XEngine/Renderer/Mesh.h>
#include <XEngine/RHI/RHITypes.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class RHIBuffer;
    class RHIDevice;
    struct MeshAsset;

    struct RenderSubmesh
    {
        u32 FirstIndex = 0;
        u32 IndexCount = 0;
        i32 VertexOffset = 0;
        u32 MaterialSlot = 0;
    };

    struct RenderMesh
    {
        std::string Name;
        std::shared_ptr<RHIBuffer> VertexBuffer;
        std::shared_ptr<RHIBuffer> IndexBuffer;
        std::vector<RenderSubmesh> Submeshes;
        u32 VertexCount = 0;
        u32 IndexCount = 0;
        RHIIndexFormat IndexFormat = RHIIndexFormat::UInt32;
        u32 Generation = 0;
    };

    class RenderMeshManager
    {
    public:
        void Initialize(RHIDevice* device);
        void Shutdown();

        MeshHandle CreateMeshFromAsset(const MeshAsset& asset);
        MeshHandle GetOrCreateMeshFromAsset(AssetHandle assetHandle, const MeshAsset& asset);

        RenderMesh* GetMesh(MeshHandle handle);
        const RenderMesh* GetMesh(MeshHandle handle) const;
        bool IsValid(MeshHandle handle) const;

    private:
        MeshHandle AddRenderMesh(RenderMesh mesh);

        RHIDevice* m_Device = nullptr;
        std::vector<RenderMesh> m_Meshes;
        std::unordered_map<u64, MeshHandle> m_AssetMeshCache;
        bool m_Initialized = false;
    };
}
