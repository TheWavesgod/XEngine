#pragma once

#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Components/LightComponent.h>
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

        void Clear();
        Entity CreateEntity(const std::string& name = {});
        void DestroyEntity(Entity entity);

        bool IsValid(Entity entity) const;
        void SetEntityName(Entity entity, const std::string& name);
        const std::string& GetEntityName(Entity entity) const;

        TransformComponent& AddTransform(Entity entity);
        MeshRendererComponent& AddMeshRenderer(Entity entity);
        CameraComponent& AddCamera(Entity entity);
        LightComponent& AddLight(Entity entity);


        TransformComponent* GetTransform(Entity entity);
        const TransformComponent* GetTransform(Entity entity) const;

        MeshRendererComponent* GetMeshRenderer(Entity entity);
        const MeshRendererComponent* GetMeshRenderer(Entity entity) const;

        CameraComponent* GetCamera(Entity entity);
        const CameraComponent* GetCamera(Entity entity) const;

        LightComponent* GetLight(Entity entity);
        const LightComponent* GetLight(Entity entity) const;

        void SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
        void ClearParent(Entity child, bool keepWorldTransform = true);
        bool HasParent(Entity entity) const;
        Entity GetParent(Entity entity) const;
        const std::vector<Entity>& GetChildren(Entity entity) const;
        std::vector<Entity> GetRootEntities() const;

        void SetWorldPosition(Entity entity, const Vec3& position);
        void SetWorldRotation(Entity entity, const Quat& rotation);
        void SetWorldRotationDegrees(Entity entity, const Math::Rotator& rotation);
        void SetWorldScale(Entity entity, const Vec3& scale);

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
        std::unordered_map<u32, LightComponent> m_Lights;
        std::unordered_map<u32, Entity> m_Parents;
        std::unordered_map<u32, std::vector<Entity>> m_Children;
    };
}
