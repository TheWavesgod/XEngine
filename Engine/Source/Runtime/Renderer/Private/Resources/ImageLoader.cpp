#include "ImageLoader.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    ImageData ImageLoader::LoadRGBA8(const std::string& path, bool flipVertically)
    {
        (void)flipVertically;
        XENGINE_LOG_WARN(std::string("Deprecated renderer image load requested: ") + path);
        return {};
    }
}
