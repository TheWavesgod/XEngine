#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ImageImporter.h"

#include <XEngine/Asset/Assets/TextureAsset.h>

#include <cstring>
#include <string>

namespace XEngine
{
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

        const std::size_t pixelSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
        texture.Width = static_cast<u32>(width);
        texture.Height = static_cast<u32>(height);
        texture.Channels = 4;
        texture.Format = TextureAssetFormat::RGBA8;
        texture.IsSRGB = true;
        texture.SourcePath = sourcePath.lexically_normal().generic_string();
        texture.Pixels.resize(pixelSize);
        std::memcpy(texture.Pixels.data(), pixels, pixelSize);

        stbi_image_free(pixels);
        return texture;
    }
}
