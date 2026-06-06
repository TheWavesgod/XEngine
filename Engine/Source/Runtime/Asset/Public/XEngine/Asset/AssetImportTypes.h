#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Asset/AssetTypes.h>

#include <filesystem>
#include <string>
#include <vector>

namespace XEngine
{
    // Public import request data passed through AssetSystem. Importer implementations stay private.
    struct AssetImportContext
    {
        std::filesystem::path SourcePath;
        AssetType RequestedType = AssetType::Unknown;
    };

    // Public result from AssetSystem::ImportAsset. Handles refer to runtime metadata, not persistent IDs.
    struct AssetImportResult
    {
        AssetImportResultCode Code = AssetImportResultCode::Failed;
        AssetHandle MainAsset;
        std::vector<AssetHandle> ImportedAssets;
        std::string Diagnostics;

        bool Succeeded() const
        {
            return Code == AssetImportResultCode::Success;
        }
    };

    // Stage 7B placeholder for future CPU-side load APIs.
    struct AssetLoadResult
    {
        AssetImportResultCode Code = AssetImportResultCode::Failed;
        AssetHandle Asset;
        std::string Diagnostics;

        bool Succeeded() const
        {
            return Code == AssetImportResultCode::Success;
        }
    };
}
