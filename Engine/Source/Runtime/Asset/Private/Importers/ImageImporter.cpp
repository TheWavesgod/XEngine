#include "ImageImporter.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/TextureAsset.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace XEngine
{
    namespace
    {
        bool IsSupportedImageExtension(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                });

            return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                   extension == ".tga" || extension == ".bmp";
        }
    }

    ImageImporter::ImageImporter(AssetSystem& assetSystem)
        : m_AssetSystem(assetSystem)
    {
    }

    std::string_view ImageImporter::GetName() const
    {
        return "ImageImporter";
    }

    std::vector<std::string_view> ImageImporter::GetSupportedExtensions() const
    {
        return { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };
    }

    bool ImageImporter::CanImport(const AssetImportContext& context) const
    {
        return context.RequestedType == AssetType::Texture &&
               IsSupportedImageExtension(context.SourcePath) &&
               std::filesystem::exists(context.SourcePath);
    }

    AssetImportResult ImageImporter::Import(const AssetImportContext& context)
    {
        AssetImportResult result;
        std::string diagnostics;
        TextureAsset texture = LoadTextureAssetRGBA8(context.SourcePath, &diagnostics);
        if (!texture.IsValid())
        {
            result.Code = AssetImportResultCode::InvalidData;
            result.Diagnostics = diagnostics.empty() ? "Image importer failed to decode texture." : diagnostics;
            return result;
        }

        result.MainAsset = m_AssetSystem.RegisterTextureAsset(context.SourcePath, std::move(texture));
        result.ImportedAssets.push_back(result.MainAsset);
        result.Code = result.MainAsset.IsValid() ? AssetImportResultCode::Success : AssetImportResultCode::Failed;
        if (!result.Succeeded())
        {
            result.Diagnostics = "Image importer decoded texture but AssetSystem registration failed.";
        }

        return result;
    }
}
