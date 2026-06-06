#include "ImageLoader.h"

#include <XEngine/Logging/Log.h>

#include <stb_image.h>

#include <cstring>
#include <string>

namespace XEngine
{
    ImageData ImageLoader::LoadRGBA8(const std::string& path, bool flipVertically)
    {
        ImageData image;

        stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            XENGINE_LOG_WARN(std::string("Failed to load image: ") + path + " (" + stbi_failure_reason() + ")");
            return image;
        }

        const std::size_t pixelSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
        image.Width = static_cast<u32>(width);
        image.Height = static_cast<u32>(height);
        image.Channels = 4;
        image.Format = ImageFormat::RGBA8;
        image.Pixels.resize(pixelSize);
        std::memcpy(image.Pixels.data(), pixels, pixelSize);

        stbi_image_free(pixels);

        XENGINE_LOG_INFO(
            std::string("Loaded image: ") + path + " " +
            std::to_string(image.Width) + "x" + std::to_string(image.Height) +
            " channels " + std::to_string(channels) + " -> 4");

        return image;
    }
}
