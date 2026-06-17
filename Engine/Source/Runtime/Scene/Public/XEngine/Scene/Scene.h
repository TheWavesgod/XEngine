#pragma once

#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Components/LightComponent.h>
#include <XEngine/Scene/Entity.h>

#include <span>
#include <string>
#include <string_view>
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
        Entity CreateEntity(std::string_view name = {});
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

        bool SetParent(Entity child, Entity parent, bool keepWorldTransform = false);
        bool ClearParent(Entity child, bool keepWorldTransform = false);
        bool HasParent(Entity entity) const;
        Entity GetParent(Entity entity) const;
        std::span<const Entity> GetChildren(Entity entity) const;
        std::span<const Entity> GetRootEntities() const;
        bool IsDescendantOf(Entity entity, Entity possibleAncestor) const;

        void SetWorldPosition(Entity entity, const Vec3& position);
        void SetWorldRotation(Entity entity, const Quat& rotation);
        void SetWorldRotationDegrees(Entity entity, const Math::Rotator& rotation);
        void SetWorldScale(Entity entity, const Vec3& scale);
        void SetLocalPosition(Entity entity, const Vec3& position);
        void SetLocalRotation(Entity entity, const Quat& rotation);
        void SetLocalRotationDegrees(Entity entity, const Math::Rotator& rotation);
        void SetLocalScale(Entity entity, const Vec3& scale);

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

        std::unordered_map<u32, Entity> m_Parents;
        std::unordered_map<u32, std::vector<Entity>> m_Children;
        // m_RootEntities is the child list of the implicit SceneRoot. The root
        // is UI/traversal-only and is not an Entity or serialized component.
        std::vector<Entity> m_RootEntities;

        std::unordered_map<u32, TransformComponent> m_Transforms;
        std::unordered_map<u32, MeshRendererComponent> m_MeshRenderers;
        std::unordered_map<u32, CameraComponent> m_Cameras;
        std::unordered_map<u32, LightComponent> m_Lights;
    };
}
