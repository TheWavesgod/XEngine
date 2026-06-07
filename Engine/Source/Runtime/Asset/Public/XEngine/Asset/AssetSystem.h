#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Asset/AssetImportTypes.h>
#include <XEngine/Asset/AssetMetadata.h>
#include <XEngine/Asset/AssetTypes.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Engine/Subsystem.h>

#include <filesystem>
#include <memory>
#include <string>

namespace XEngine
{
    class AssetRegistry;
    class ImporterRegistry;

    // Runtime subsystem for source asset metadata and CPU-side asset data.
    // It does not own Renderer managers and never creates RHI/GPU resources.
    class AssetSystem final : public ISubsystem
    {
    public:
        AssetSystem();
        ~AssetSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;

        AssetHandle RegisterSourceAsset(
            const std::filesystem::path& sourcePath,
            AssetType type);

        const AssetMetadata* GetMetadata(AssetHandle handle) const;
        AssetMetadata* GetMetadata(AssetHandle handle);

        AssetHandle FindAssetByPath(const std::filesystem::path& sourcePath) const;
        const AssetMetadata* FindMetadataByPath(const std::filesystem::path& sourcePath) const;

        AssetImportResult ImportAsset(const std::filesystem::path& sourcePath);

        const MeshAsset* GetMeshAsset(AssetHandle handle) const;
        const MaterialAsset* GetMaterialAsset(AssetHandle handle) const;
        const TextureAsset* GetTextureAsset(AssetHandle handle) const;

        // Creates a CPU-side procedural cube MeshAsset for Stage 7C renderer validation.
        // Real mesh import through glTF is introduced in Stage 7E.
        AssetHandle CreateProceduralCubeMeshAsset(const std::string& name);

        // Creates a CPU-side test MaterialAsset for renderer validation.
        // This temporary Stage 7D helper is replaced by glTF material import in Stage 7E.
        AssetHandle CreateTestMaterialAsset(
            const std::string& name,
            AssetHandle baseColorTexture);

        AssetType GuessAssetTypeFromPath(const std::filesystem::path& sourcePath) const;

        std::size_t GetAssetCount() const;

        // Private-to-Asset module entry point used by Stage 7B ImageImporter.
        AssetHandle RegisterTextureAsset(
            const std::filesystem::path& sourcePath,
            TextureAsset textureAsset);

        // Private-to-Asset module entry point for future mesh importers.
        AssetHandle RegisterMeshAsset(
            const std::filesystem::path& sourcePath,
            MeshAsset meshAsset);

        // Private-to-Asset module entry point for future material importers.
        AssetHandle RegisterMaterialAsset(
            const std::filesystem::path& sourcePath,
            MaterialAsset materialAsset);

    private:
        struct Impl;

        void RegisterValidationAssetMetadata();

        std::unique_ptr<Impl> m_Impl;
    };
}
