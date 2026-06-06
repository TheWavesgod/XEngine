#pragma once

#include <XEngine/Core/Types.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class ImageFormat
    {
        Unknown,
        RGBA8,
        RGBA32Float
    };

    struct ImageData
    {
        u32 Width = 0;
        u32 Height = 0;
        u32 Channels = 0;
        ImageFormat Format = ImageFormat::Unknown;
        std::vector<u8> Pixels;

        bool IsValid() const
        {
            return Width > 0 && Height > 0 && !Pixels.empty();
        }
    };

    class ImageLoader
    {
    public:
        static ImageData LoadRGBA8(const std::string& path, bool flipVertically = true);
    };
}
