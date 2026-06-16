#pragma once

#include <XEngine/Serialization/SerializationContext.h>

#include <filesystem>

namespace XEngine
{
    class Scene;

    // SceneSerializer lives in the Scene module because it understands Scene
    // entities, components, hierarchy, and asset references. Runtime/Serialization
    // remains the generic JSON/file IO layer.
    class SceneSerializer
    {
    public:
        explicit SceneSerializer(const SerializationContext& context);

        bool LoadFromFile(Scene& scene, const std::filesystem::path& path);
        bool SaveToFile(const Scene& scene, const std::filesystem::path& path);

    private:
        SerializationContext m_Context;
    };
}
