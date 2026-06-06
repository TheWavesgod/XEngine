#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Asset/AssetTypes.h>

#include <filesystem>
#include <string>
#include <vector>

namespace XEngine
{
    // Describes a source-level dependency. Stage 7A records the shape but does not populate it yet.
    struct AssetDependency
    {
        AssetHandle Handle;
        std::filesystem::path Path;
        AssetType Type = AssetType::Unknown;
    };

    // Runtime metadata for a source asset. It is owned by AssetSystem and is not a cooked database entry.
    struct AssetMetadata
    {
        AssetHandle Handle;
        AssetType Type = AssetType::Unknown;

        std::filesystem::path SourcePath;
        std::string Name;

        AssetLoadState LoadState = AssetLoadState::Unloaded;

        std::vector<AssetDependency> Dependencies;
    };
}
