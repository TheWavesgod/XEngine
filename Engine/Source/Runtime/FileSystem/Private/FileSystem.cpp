#include <XEngine/FileSystem/FileSystem.h>

#include <filesystem>

namespace XEngine
{
    bool FileSystem::Exists(const char* path) const
    {
        return std::filesystem::exists(path);
    }
}

