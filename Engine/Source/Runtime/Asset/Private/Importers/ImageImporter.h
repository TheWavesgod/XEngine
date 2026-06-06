#pragma once

#include "AssetImporter.h"

#include <filesystem>
#include <string_view>
#include <vector>

namespace XEngine
{
    class AssetSystem;
    struct TextureAsset;

    class ImageImporter final : public IAssetImporter
    {
    public:
        explicit ImageImporter(AssetSystem& assetSystem);

        std::string_view GetName() const override;
        std::vector<std::string_view> GetSupportedExtensions() const override;
        bool CanImport(const AssetImportContext& context) const override;
        AssetImportResult Import(const AssetImportContext& context) override;

    private:
        AssetSystem& m_AssetSystem;
    };

    TextureAsset LoadTextureAssetRGBA8(const std::filesystem::path& sourcePath, std::string* diagnostics);
}
