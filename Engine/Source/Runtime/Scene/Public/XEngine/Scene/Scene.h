#pragma once

#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/Scene/Entity.h>

#include <string>

namespace XEngine
{
    class Scene
    {
    public:
        Entity CreateEntity(const std::string& name);
        void DestroyEntity(Entity entity);
        RenderScene ExtractRenderScene() const;

    private:
        unsigned int m_NextEntityId = 1;
    };
}
