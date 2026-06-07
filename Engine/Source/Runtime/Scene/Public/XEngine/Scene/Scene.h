#pragma once

#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Entity.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    // Minimal ECS-style scene container owned by SceneSystem.
    // Scene stores CPU-side component data and AssetHandle references only.
    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name = {});
        void DestroyEntity(Entity entity);

        bool IsValid(Entity entity) const;

        TransformComponent& AddTransform(Entity entity);
        MeshRendererComponent& AddMeshRenderer(Entity entity);
        CameraComponent& AddCamera(Entity entity);

        TransformComponent* GetTransform(Entity entity);
        const TransformComponent* GetTransform(Entity entity) const;

        MeshRendererComponent* GetMeshRenderer(Entity entity);
        const MeshRendererComponent* GetMeshRenderer(Entity entity) const;

        CameraComponent* GetCamera(Entity entity);
        const CameraComponent* GetCamera(Entity entity) const;

        const std::vector<Entity>& GetEntities() const;

        void UpdateTransforms();

    private:
        struct EntityRecord
        {
            u32 Generation = 0;
            bool Alive = false;
            std::string Name;
        };

        std::vector<EntityRecord> m_EntityRecords;
        std::vector<Entity> m_Entities;
        std::unordered_map<u32, TransformComponent> m_Transforms;
        std::unordered_map<u32, MeshRendererComponent> m_MeshRenderers;
        std::unordered_map<u32, CameraComponent> m_Cameras;
    };
}
