#pragma once

#include <XEngine/Asset/AssetImportTypes.h>

#include <string_view>
#include <vector>

namespace XEngine
{
    // Private interface used by AssetSystem. Other modules call AssetSystem::ImportAsset instead.
    class IAssetImporter
    {
    public:
        virtual ~IAssetImporter() = default;

        virtual std::string_view GetName() const = 0;
        virtual std::vector<std::string_view> GetSupportedExtensions() const = 0;
        virtual bool CanImport(const AssetImportContext& context) const = 0;
        virtual AssetImportResult Import(const AssetImportContext& context) = 0;
    };
}
