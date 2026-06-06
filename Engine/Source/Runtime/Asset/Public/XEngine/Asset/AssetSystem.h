#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Asset/AssetImportTypes.h>
#include <XEngine/Asset/AssetMetadata.h>
#include <XEngine/Asset/AssetTypes.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Engine/Subsystem.h>

#include <filesystem>
#include <memory>
#include <unordered_map>

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

        const TextureAsset* GetTextureAsset(AssetHandle handle) const;

        AssetType GuessAssetTypeFromPath(const std::filesystem::path& sourcePath) const;

        std::size_t GetAssetCount() const;

        // Private-to-Asset module entry point used by Stage 7B ImageImporter.
        AssetHandle RegisterTextureAsset(
            const std::filesystem::path& sourcePath,
            TextureAsset textureAsset);

    private:
        void RegisterValidationAssetMetadata();

        std::unique_ptr<AssetRegistry> m_Registry;
        std::unique_ptr<ImporterRegistry> m_ImporterRegistry;
        std::unordered_map<u32, TextureAsset> m_TextureAssets;

        bool m_Initialized = false;
    };
}
