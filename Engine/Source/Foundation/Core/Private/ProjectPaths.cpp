#include <XEngine/Core/ProjectPaths.h>

#include <XEngine/Core/Generated/XEngineBuildPaths.h>
#include <XEngine/Logging/Log.h>

#include <array>
#include <string>

namespace XEngine
{
    namespace
    {
        struct PathState
        {
            std::filesystem::path EngineRoot;
            std::filesystem::path ProjectRoot;
            std::filesystem::path AssetRoot;
            std::filesystem::path ShaderRoot;
            std::filesystem::path ConfigRoot;
            std::filesystem::path SavedRoot;
            std::filesystem::path CacheRoot;
            bool Initialized = false;
        };

        PathState& GetState()
        {
            static PathState state;
            return state;
        }

        std::filesystem::path Normalize(const std::filesystem::path& path)
        {
            std::error_code error;
            const std::filesystem::path normalized = path.lexically_normal();
            const std::filesystem::path canonical = std::filesystem::weakly_canonical(normalized, error);
            return error ? normalized : canonical;
        }

        bool ConsumePrefix(std::string_view& path, std::string_view prefix)
        {
            if (!path.starts_with(prefix))
            {
                return false;
            }

            path.remove_prefix(prefix.size());
            return true;
        }

        std::filesystem::path Join(const std::filesystem::path& root, std::string_view relativePath)
        {
            std::filesystem::path path(relativePath);
            if (path.is_absolute())
            {
                return Normalize(path);
            }

            return Normalize(root / path);
        }

        void LogPath(const char* name, const std::filesystem::path& path)
        {
            XENGINE_LOG_INFO(std::string(name) + ": " + path.generic_string());
        }
    }

    void ProjectPaths::Initialize()
    {
        PathState& state = GetState();
        if (state.Initialized)
        {
            return;
        }

        state.EngineRoot = Normalize(BuildPaths::DevEngineRoot);
        state.ProjectRoot = Normalize(BuildPaths::DevProjectRoot);
        state.AssetRoot = Normalize(state.ProjectRoot / "Assets");
        state.ShaderRoot = Normalize(state.EngineRoot / "Shaders");
        state.ConfigRoot = Normalize(state.ProjectRoot / "Config");
        state.SavedRoot = Normalize(state.ProjectRoot / "Saved");
        state.CacheRoot = Normalize(state.SavedRoot / "Cache");
        state.Initialized = true;

        LogPath("EngineRoot", state.EngineRoot);
        LogPath("ProjectRoot", state.ProjectRoot);
        LogPath("AssetRoot", state.AssetRoot);
        LogPath("ShaderRoot", state.ShaderRoot);
        LogPath("ConfigRoot", state.ConfigRoot);
        LogPath("SavedRoot", state.SavedRoot);
        LogPath("CacheRoot", state.CacheRoot);
    }

    const std::filesystem::path& ProjectPaths::GetEngineRoot()
    {
        Initialize();
        return GetState().EngineRoot;
    }

    const std::filesystem::path& ProjectPaths::GetProjectRoot()
    {
        Initialize();
        return GetState().ProjectRoot;
    }

    const std::filesystem::path& ProjectPaths::GetAssetRoot()
    {
        Initialize();
        return GetState().AssetRoot;
    }

    const std::filesystem::path& ProjectPaths::GetShaderRoot()
    {
        Initialize();
        return GetState().ShaderRoot;
    }

    const std::filesystem::path& ProjectPaths::GetConfigRoot()
    {
        Initialize();
        return GetState().ConfigRoot;
    }

    const std::filesystem::path& ProjectPaths::GetSavedRoot()
    {
        Initialize();
        return GetState().SavedRoot;
    }

    const std::filesystem::path& ProjectPaths::GetCacheRoot()
    {
        Initialize();
        return GetState().CacheRoot;
    }

    std::filesystem::path ProjectPaths::Resolve(std::string_view virtualPath)
    {
        Initialize();

        std::string_view path = virtualPath;
        // Virtual path mapping:
        // project:// -> ProjectRoot, engine:// -> EngineRoot,
        // asset:// -> ProjectRoot/Assets, shader:// -> EngineRoot/Shaders,
        // config:// -> ProjectRoot/Config, saved:// -> ProjectRoot/Saved,
        // cache:// -> ProjectRoot/Saved/Cache.
        if (ConsumePrefix(path, "project://"))
        {
            return ResolveProjectPath(path);
        }
        if (ConsumePrefix(path, "engine://"))
        {
            return ResolveEnginePath(path);
        }
        if (ConsumePrefix(path, "asset://"))
        {
            return Join(GetAssetRoot(), path);
        }
        if (ConsumePrefix(path, "shader://"))
        {
            return Join(GetShaderRoot(), path);
        }
        if (ConsumePrefix(path, "config://"))
        {
            return ResolveConfigPath(path);
        }
        if (ConsumePrefix(path, "saved://"))
        {
            return ResolveSavedPath(path);
        }
        if (ConsumePrefix(path, "cache://"))
        {
            return ResolveCachePath(path);
        }

        std::filesystem::path rawPath(virtualPath);
        if (!rawPath.is_absolute())
        {
            XENGINE_LOG_WARN(std::string("Resolving raw relative path as project-relative: ") + std::string(virtualPath));
        }
        return rawPath.is_absolute() ? Normalize(rawPath) : ResolveProjectPath(virtualPath);
    }

    std::filesystem::path ProjectPaths::ResolveProjectPath(std::string_view relativePath)
    {
        return Join(GetProjectRoot(), relativePath);
    }

    std::filesystem::path ProjectPaths::ResolveEnginePath(std::string_view relativePath)
    {
        return Join(GetEngineRoot(), relativePath);
    }

    std::filesystem::path ProjectPaths::ResolveConfigPath(std::string_view relativePath)
    {
        return Join(GetConfigRoot(), relativePath);
    }

    std::filesystem::path ProjectPaths::ResolveSavedPath(std::string_view relativePath)
    {
        return Join(GetSavedRoot(), relativePath);
    }

    std::filesystem::path ProjectPaths::ResolveCachePath(std::string_view relativePath)
    {
        return Join(GetCacheRoot(), relativePath);
    }
}
