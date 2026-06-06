#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Asset/AssetMetadata.h>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class AssetRegistry
    {
    public:
        AssetHandle RegisterAsset(const AssetMetadata& metadata);

        const AssetMetadata* GetMetadata(AssetHandle handle) const;
        AssetMetadata* GetMetadata(AssetHandle handle);

        const AssetMetadata* FindByPath(const std::filesystem::path& path) const;
        AssetHandle FindHandleByPath(const std::filesystem::path& path) const;

        bool IsValid(AssetHandle handle) const;

        std::size_t GetAssetCount() const;

    private:
        static std::string MakePathKey(const std::filesystem::path& path);

        std::vector<AssetMetadata> m_Metadata;
        std::unordered_map<std::string, AssetHandle> m_PathToHandle;
    };
}
