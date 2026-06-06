#include <XEngine/Asset/AssetSystem.h>

#include "AssetRegistry.h"
#include "Importers/ImageImporter.h"
#include "Importers/ImporterRegistry.h"

#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <utility>

namespace XEngine
{
    namespace
    {
        std::string ToLower(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                });
            return value;
        }
    }

    AssetSystem::AssetSystem() = default;

    AssetSystem::~AssetSystem()
    {
        OnDestroy();
    }

    void AssetSystem::OnCreate(const SubsystemContext&)
    {
        if (m_Initialized)
        {
            return;
        }

        m_Registry = std::make_unique<AssetRegistry>();
        m_ImporterRegistry = std::make_unique<ImporterRegistry>();
        m_ImporterRegistry->RegisterImporter(std::make_unique<ImageImporter>(*this));

        RegisterValidationAssetMetadata();

        m_Initialized = true;

        std::string message = "AssetSystem initialized with ";
        message += std::to_string(GetAssetCount());
        message += " metadata entries";
        XENGINE_LOG_INFO(message);
    }

    void AssetSystem::OnDestroy()
    {
        if (!m_Initialized && !m_Registry && !m_ImporterRegistry && m_TextureAssets.empty())
        {
            return;
        }

        XENGINE_LOG_INFO("AssetSystem shutdown");
        m_TextureAssets.clear();
        m_ImporterRegistry.reset();
        m_Registry.reset();
        m_Initialized = false;
    }

    AssetHandle AssetSystem::RegisterSourceAsset(
        const std::filesystem::path& sourcePath,
        AssetType type)
    {
        if (!m_Registry)
        {
            return {};
        }

        const AssetHandle existing = m_Registry->FindHandleByPath(sourcePath);
        if (existing.IsValid())
        {
            return existing;
        }

        AssetMetadata metadata;
        metadata.SourcePath = sourcePath.lexically_normal();
        metadata.Type = type;
        metadata.Name = metadata.SourcePath.stem().string();
        metadata.LoadState = AssetLoadState::Unloaded;

        return m_Registry->RegisterAsset(metadata);
    }

    const AssetMetadata* AssetSystem::GetMetadata(AssetHandle handle) const
    {
        return m_Registry ? m_Registry->GetMetadata(handle) : nullptr;
    }

    AssetMetadata* AssetSystem::GetMetadata(AssetHandle handle)
    {
        return m_Registry ? m_Registry->GetMetadata(handle) : nullptr;
    }

    AssetHandle AssetSystem::FindAssetByPath(const std::filesystem::path& sourcePath) const
    {
        return m_Registry ? m_Registry->FindHandleByPath(sourcePath) : AssetHandle {};
    }

    const AssetMetadata* AssetSystem::FindMetadataByPath(const std::filesystem::path& sourcePath) const
    {
        return m_Registry ? m_Registry->FindByPath(sourcePath) : nullptr;
    }

    AssetImportResult AssetSystem::ImportAsset(const std::filesystem::path& sourcePath)
    {
        AssetImportResult result;
        if (!std::filesystem::exists(sourcePath))
        {
            result.Code = AssetImportResultCode::FileNotFound;
            result.Diagnostics = "Asset source file was not found.";
            return result;
        }

        AssetImportContext context;
        context.SourcePath = sourcePath.lexically_normal();
        context.RequestedType = GuessAssetTypeFromPath(context.SourcePath);

        if (!m_ImporterRegistry)
        {
            result.Code = AssetImportResultCode::ImporterUnavailable;
            result.Diagnostics = "Asset importer registry is not initialized.";
            return result;
        }

        // Stage 7B dispatches importers by extension. CanImport() remains as a
        // final validation hook rather than the primary lookup mechanism.
        IAssetImporter* importer = m_ImporterRegistry->FindImporterForExtension(context.SourcePath.extension().string());
        if (importer == nullptr)
        {
            result.Code = AssetImportResultCode::ImporterUnavailable;
            result.Diagnostics = "No registered importer handles this asset extension.";
            return result;
        }

        if (!importer->CanImport(context))
        {
            result.Code = AssetImportResultCode::UnsupportedFormat;
            result.Diagnostics = "Registered importer rejected this asset source.";
            return result;
        }

        return importer->Import(context);
    }

    const TextureAsset* AssetSystem::GetTextureAsset(AssetHandle handle) const
    {
        const AssetMetadata* metadata = GetMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Texture)
        {
            return nullptr;
        }

        const auto it = m_TextureAssets.find(handle.Index);
        if (it == m_TextureAssets.end())
        {
            return nullptr;
        }

        return it->second.IsValid() ? &it->second : nullptr;
    }

    AssetHandle AssetSystem::RegisterTextureAsset(
        const std::filesystem::path& sourcePath,
        TextureAsset textureAsset)
    {
        if (!textureAsset.IsValid())
        {
            return {};
        }

        AssetHandle handle = RegisterSourceAsset(sourcePath, AssetType::Texture);
        if (!handle.IsValid())
        {
            return {};
        }

        if (AssetMetadata* metadata = GetMetadata(handle))
        {
            metadata->LoadState = AssetLoadState::Loaded;
            metadata->Type = AssetType::Texture;
        }

        m_TextureAssets[handle.Index] = std::move(textureAsset);
        return handle;
    }

    AssetType AssetSystem::GuessAssetTypeFromPath(const std::filesystem::path& sourcePath) const
    {
        if (const AssetMetadata* metadata = FindMetadataByPath(sourcePath))
        {
            return metadata->Type;
        }

        const std::string extension = ToLower(sourcePath.extension().string());

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
            extension == ".tga" || extension == ".bmp")
        {
            return AssetType::Texture;
        }

        if (extension == ".gltf" || extension == ".glb")
        {
            return AssetType::Gltf;
        }

        if (extension == ".slang")
        {
            return AssetType::Shader;
        }

        return AssetType::Unknown;
    }

    std::size_t AssetSystem::GetAssetCount() const
    {
        return m_Registry ? m_Registry->GetAssetCount() : 0;
    }

    void AssetSystem::RegisterValidationAssetMetadata()
    {
        const std::filesystem::path validationRoot = "Assets/models/gltf";
        if (!std::filesystem::exists(validationRoot))
        {
            return;
        }

        // Stage 7A records source metadata only. Parsing these glTF files starts in Stage 7E.
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::recursive_directory_iterator(validationRoot))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const AssetType type = GuessAssetTypeFromPath(entry.path());
            if (type == AssetType::Unknown)
            {
                continue;
            }

            RegisterSourceAsset(entry.path(), type);
        }
    }
}
