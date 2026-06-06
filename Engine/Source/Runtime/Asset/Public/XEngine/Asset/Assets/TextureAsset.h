#pragma once

#include <XEngine/Core/Types.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class TextureAssetFormat
    {
        Unknown,
        RGBA8,
        RGBA32Float
    };

    // CPU-side texture data imported by AssetSystem. It owns no GPU resources.
    struct TextureAsset
    {
        u32 Width = 0;
        u32 Height = 0;
        u32 Channels = 0;

        TextureAssetFormat Format = TextureAssetFormat::Unknown;
        bool IsSRGB = true;

        std::string SourcePath;
        std::vector<u8> Pixels;

        bool IsValid() const
        {
            return Width > 0 && Height > 0 && !Pixels.empty();
        }
    };
}
