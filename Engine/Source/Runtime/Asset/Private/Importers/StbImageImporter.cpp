#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ImageImporter.h"

#include <XEngine/Asset/Assets/TextureAsset.h>

#include <cstring>
#include <limits>
#include <span>
#include <string>

namespace XEngine
{
    namespace
    {
        TextureAsset BuildTextureAssetRGBA8(
            stbi_uc* pixels,
            int width,
            int height,
            const std::string& sourceName,
            bool isSRGB)
        {
            TextureAsset texture;
            if (pixels == nullptr || width <= 0 || height <= 0)
            {
                return texture;
            }

            const std::size_t pixelSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
            texture.Width = static_cast<u32>(width);
            texture.Height = static_cast<u32>(height);
            texture.Channels = 4;
            texture.Format = TextureAssetFormat::RGBA8;
            texture.IsSRGB = isSRGB;
            texture.SourcePath = sourceName;
            texture.Pixels.resize(pixelSize);
            std::memcpy(texture.Pixels.data(), pixels, pixelSize);
            return texture;
        }
    }

    TextureAsset LoadTextureAssetRGBA8(const std::filesystem::path& sourcePath, std::string* diagnostics)
    {
        TextureAsset texture;
        const std::string path = sourcePath.lexically_normal().string();

        stbi_set_flip_vertically_on_load(1);

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            if (diagnostics != nullptr)
            {
                *diagnostics = std::string("Failed to load image: ") + path + " (" + stbi_failure_reason() + ")";
            }
            return texture;
        }

        texture = BuildTextureAssetRGBA8(
            pixels,
            width,
            height,
            sourcePath.lexically_normal().generic_string(),
            true);

        stbi_image_free(pixels);
        return texture;
    }

    TextureAsset LoadTextureAssetRGBA8FromMemory(
        std::span<const std::byte> bytes,
        const std::string& sourceName,
        bool isSRGB,
        std::string* diagnostics)
    {
        TextureAsset texture;
        if (bytes.empty() || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
            if (diagnostics != nullptr)
            {
                *diagnostics = "Image memory buffer is empty or too large.";
            }
            return texture;
        }

        stbi_set_flip_vertically_on_load(1);

        int width = 0;
        int height = 0;
        int channels = 0;
        const auto* data = reinterpret_cast<const stbi_uc*>(bytes.data());
        stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(bytes.size()), &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            if (diagnostics != nullptr)
            {
                *diagnostics = std::string("Failed to decode image memory: ") + sourceName + " (" + stbi_failure_reason() + ")";
            }
            return texture;
        }

        texture = BuildTextureAssetRGBA8(pixels, width, height, sourceName, isSRGB);
        stbi_image_free(pixels);
        return texture;
    }
}
