#pragma once

#include <filesystem>
#include <string_view>

namespace XEngine
{
    class ProjectPaths
    {
    public:
        static void Initialize();

        static const std::filesystem::path& GetEngineRoot();
        static const std::filesystem::path& GetProjectRoot();

        static const std::filesystem::path& GetAssetRoot();
        static const std::filesystem::path& GetShaderRoot();
        static const std::filesystem::path& GetConfigRoot();
        static const std::filesystem::path& GetSavedRoot();
        static const std::filesystem::path& GetCacheRoot();

        static std::filesystem::path Resolve(std::string_view virtualPath);
        static std::filesystem::path ResolveProjectPath(std::string_view relativePath);
        static std::filesystem::path ResolveEnginePath(std::string_view relativePath);
        static std::filesystem::path ResolveConfigPath(std::string_view relativePath);
        static std::filesystem::path ResolveSavedPath(std::string_view relativePath);
        static std::filesystem::path ResolveCachePath(std::string_view relativePath);
    };
}
