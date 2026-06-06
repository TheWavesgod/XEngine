#include "AssetRegistry.h"

#include <utility>

namespace XEngine
{
    AssetHandle AssetRegistry::RegisterAsset(const AssetMetadata& metadata)
    {
        const std::string pathKey = MakePathKey(metadata.SourcePath);
        const auto existing = m_PathToHandle.find(pathKey);
        if (existing != m_PathToHandle.end())
        {
            return existing->second;
        }

        AssetMetadata storedMetadata = metadata;

        AssetHandle handle;
        handle.Index = static_cast<u32>(m_Metadata.size());
        handle.Generation = 1;
        storedMetadata.Handle = handle;

        m_Metadata.push_back(std::move(storedMetadata));
        if (!pathKey.empty())
        {
            m_PathToHandle.emplace(pathKey, handle);
        }

        return handle;
    }

    const AssetMetadata* AssetRegistry::GetMetadata(AssetHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &m_Metadata[handle.Index];
    }

    AssetMetadata* AssetRegistry::GetMetadata(AssetHandle handle)
    {
        return const_cast<AssetMetadata*>(static_cast<const AssetRegistry*>(this)->GetMetadata(handle));
    }

    const AssetMetadata* AssetRegistry::FindByPath(const std::filesystem::path& path) const
    {
        return GetMetadata(FindHandleByPath(path));
    }

    AssetHandle AssetRegistry::FindHandleByPath(const std::filesystem::path& path) const
    {
        const auto it = m_PathToHandle.find(MakePathKey(path));
        if (it == m_PathToHandle.end())
        {
            return {};
        }

        return it->second;
    }

    bool AssetRegistry::IsValid(AssetHandle handle) const
    {
        if (!handle.IsValid() || handle.Index >= m_Metadata.size())
        {
            return false;
        }

        return m_Metadata[handle.Index].Handle.Generation == handle.Generation;
    }

    std::size_t AssetRegistry::GetAssetCount() const
    {
        return m_Metadata.size();
    }

    std::string AssetRegistry::MakePathKey(const std::filesystem::path& path)
    {
        return path.lexically_normal().generic_string();
    }
}
