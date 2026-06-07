#pragma once

#include "AssetImporter.h"

#include <string_view>
#include <vector>

namespace XEngine
{
    class AssetSystem;

    // Private glTF importer. It converts source containers into CPU-side assets
    // and intentionally does not create renderer or RHI resources.
    class GltfImporter final : public IAssetImporter
    {
    public:
        explicit GltfImporter(AssetSystem& assetSystem);

        std::string_view GetName() const override;
        std::vector<std::string_view> GetSupportedExtensions() const override;
        bool CanImport(const AssetImportContext& context) const override;
        AssetImportResult Import(const AssetImportContext& context) override;

    private:
        AssetSystem& m_AssetSystem;
    };
}
