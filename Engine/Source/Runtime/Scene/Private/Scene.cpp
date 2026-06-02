#include <XEngine/Scene/Scene.h>

namespace XEngine
{
    Entity Scene::CreateEntity(const std::string& name)
    {
        (void)name;
        return Entity(m_NextEntityId++);
    }

    void Scene::DestroyEntity(Entity entity)
    {
        (void)entity;
    }

    RenderScene Scene::ExtractRenderScene() const
    {
        return {};
    }
}

