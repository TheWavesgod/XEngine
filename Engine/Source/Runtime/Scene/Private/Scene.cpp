#include <XEngine/Scene/Scene.h>

#include "Systems/TransformSystem.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Math/MathFunctions.h>

#include <algorithm>
#include <cmath>
#include <span>
#include <utility>

namespace XEngine
{
    namespace
    {
        const std::vector<Entity> EmptyChildren;
        const std::string EmptyName;

        void EraseEntity(std::vector<Entity>& entities, Entity entity)
        {
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
        }

        Vec3 SafeComponentDivide(const Vec3& value, const Vec3& divisor)
        {
            Vec3 result = value;
            for (int component = 0; component < 3; ++component)
            {
                if (std::abs(divisor[component]) > 0.000001f)
                {
                    result[component] /= divisor[component];
                }
            }
            return result;
        }

        const TransformComponent* FindInheritedParentTransform(const Scene& scene, Entity entity)
        {
            for (Entity parent = scene.GetParent(entity); parent.IsValid(); parent = scene.GetParent(parent))
            {
                if (const TransformComponent* transform = scene.GetTransform(parent))
                {
                    return transform;
                }
            }
            return nullptr;
        }
    }

    Scene::Scene() = default;
    Scene::~Scene() = default;

    void Scene::Clear()
    {
        m_EntityRecords.clear();
        m_Entities.clear();
        m_Transforms.clear();
        m_MeshRenderers.clear();
        m_Cameras.clear();
        m_Lights.clear();
        m_Parents.clear();
        m_Children.clear();
        m_RootEntities.clear();
    }

    Entity Scene::CreateEntity(std::string_view name)
    {
        EntityRecord record;
        record.Generation = 1;
        record.Alive = true;
        record.Name = std::string(name);

        Entity entity;
        entity.Index = static_cast<u32>(m_EntityRecords.size());
        entity.Generation = record.Generation;

        m_EntityRecords.push_back(std::move(record));
        m_Entities.push_back(entity);
        m_RootEntities.push_back(entity);
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!IsValid(entity))
        {
            return;
        }

        const std::vector<Entity> children(GetChildren(entity).begin(), GetChildren(entity).end());
        for (Entity child : children)
        {
            DestroyEntity(child);
        }

        if (HasParent(entity))
        {
            ClearParent(entity, false);
        }
        else
        {
            EraseEntity(m_RootEntities, entity);
        }
        m_Children.erase(entity.Index);

        EntityRecord& record = m_EntityRecords[entity.Index];
        record.Alive = false;
        ++record.Generation;
        record.Name.clear();

        m_Transforms.erase(entity.Index);
        m_MeshRenderers.erase(entity.Index);
        m_Cameras.erase(entity.Index);
        m_Lights.erase(entity.Index);

        EraseEntity(m_Entities, entity);
    }

    bool Scene::IsValid(Entity entity) const
    {
        if (!entity.IsValid() || entity.Index >= m_EntityRecords.size())
        {
            return false;
        }

        const EntityRecord& record = m_EntityRecords[entity.Index];
        return record.Alive && record.Generation == entity.Generation;
    }

    void Scene::SetEntityName(Entity entity, const std::string& name)
    {
        if (!IsValid(entity))
        {
            return;
        }

        m_EntityRecords[entity.Index].Name = name;
    }

    const std::string& Scene::GetEntityName(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return EmptyName;
        }

        return m_EntityRecords[entity.Index].Name;
    }

    TransformComponent& Scene::AddTransform(Entity entity)
    {
        XENGINE_ASSERT(IsValid(entity), "Cannot add TransformComponent to invalid entity");
        TransformComponent& transform = m_Transforms[entity.Index];
        transform.MarkDirty();
        return transform;
    }

    MeshRendererComponent& Scene::AddMeshRenderer(Entity entity)
    {
        XENGINE_ASSERT(IsValid(entity), "Cannot add MeshRendererComponent to invalid entity");
        return m_MeshRenderers[entity.Index];
    }

    CameraComponent& Scene::AddCamera(Entity entity)
    {
        XENGINE_ASSERT(IsValid(entity), "Cannot add CameraComponent to invalid entity");
        return m_Cameras[entity.Index];
    }

    LightComponent& Scene::AddLight(Entity entity)
    {
        XENGINE_ASSERT(IsValid(entity), "Cannot add LightComponent to invalid entity");
        return m_Lights[entity.Index];
    }

    TransformComponent* Scene::GetTransform(Entity entity)
    {
        return const_cast<TransformComponent*>(static_cast<const Scene*>(this)->GetTransform(entity));
    }

    const TransformComponent* Scene::GetTransform(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return nullptr;
        }
        const auto it = m_Transforms.find(entity.Index);
        return it != m_Transforms.end() ? &it->second : nullptr;
    }

    MeshRendererComponent* Scene::GetMeshRenderer(Entity entity)
    {
        return const_cast<MeshRendererComponent*>(static_cast<const Scene*>(this)->GetMeshRenderer(entity));
    }

    const MeshRendererComponent* Scene::GetMeshRenderer(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return nullptr;
        }
        const auto it = m_MeshRenderers.find(entity.Index);
        return it != m_MeshRenderers.end() ? &it->second : nullptr;
    }

    CameraComponent* Scene::GetCamera(Entity entity)
    {
        return const_cast<CameraComponent*>(static_cast<const Scene*>(this)->GetCamera(entity));
    }

    const CameraComponent* Scene::GetCamera(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return nullptr;
        }
        const auto it = m_Cameras.find(entity.Index);
        return it != m_Cameras.end() ? &it->second : nullptr;
    }

    LightComponent* Scene::GetLight(Entity entity)
    {
        return const_cast<LightComponent*>(static_cast<const Scene*>(this)->GetLight(entity));
    }

    const LightComponent* Scene::GetLight(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return nullptr;
        }
        const auto it = m_Lights.find(entity.Index);
        return it != m_Lights.end() ? &it->second : nullptr;
    }

    bool Scene::SetParent(Entity child, Entity parent, bool keepWorldTransform)
    {
        if (!IsValid(child) || !IsValid(parent) || child == parent)
        {
            return false;
        }

        // Prevent cycles before touching storage; a child cannot be reparented
        // under itself or any of its descendants.
        if (IsDescendantOf(parent, child))
        {
            return false;
        }

        UpdateTransforms();
        Vec3 worldPosition {};
        Quat worldRotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 worldScale { 1.0f, 1.0f, 1.0f };
        if (const TransformComponent* transform = GetTransform(child))
        {
            worldPosition = transform->GetWorldPosition();
            worldRotation = transform->GetWorldRotation();
            worldScale = transform->GetWorldScale();
        }

        if (HasParent(child))
        {
            ClearParent(child, false);
        }
        else
        {
            EraseEntity(m_RootEntities, child);
        }

        m_Parents[child.Index] = parent;
        EraseEntity(m_Children[parent.Index], child);
        m_Children[parent.Index].push_back(child);

        if (keepWorldTransform)
        {
            // keepWorldTransform preserves the visual world transform by
            // recomputing local values relative to the new parent.
            SetWorldPosition(child, worldPosition);
            SetWorldRotation(child, worldRotation);
            SetWorldScale(child, worldScale);
        }
        else if (TransformComponent* transform = GetTransform(child))
        {
            transform->MarkDirty();
        }
        return true;
    }

    bool Scene::ClearParent(Entity child, bool keepWorldTransform)
    {
        if (!IsValid(child))
        {
            return false;
        }

        const auto parentIt = m_Parents.find(child.Index);
        if (parentIt == m_Parents.end())
        {
            if (std::find(m_RootEntities.begin(), m_RootEntities.end(), child) == m_RootEntities.end())
            {
                m_RootEntities.push_back(child);
            }
            return true;
        }

        UpdateTransforms();
        Vec3 worldPosition {};
        Quat worldRotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 worldScale { 1.0f, 1.0f, 1.0f };
        if (const TransformComponent* transform = GetTransform(child))
        {
            worldPosition = transform->GetWorldPosition();
            worldRotation = transform->GetWorldRotation();
            worldScale = transform->GetWorldScale();
        }

        const Entity parent = parentIt->second;
        m_Parents.erase(parentIt);
        auto childrenIt = m_Children.find(parent.Index);
        if (childrenIt != m_Children.end())
        {
            auto& siblings = childrenIt->second;
            EraseEntity(siblings, child);
            if (siblings.empty())
            {
                m_Children.erase(childrenIt);
            }
        }
        if (std::find(m_RootEntities.begin(), m_RootEntities.end(), child) == m_RootEntities.end())
        {
            m_RootEntities.push_back(child);
        }

        if (keepWorldTransform)
        {
            if (TransformComponent* transform = GetTransform(child))
            {
                transform->SetLocalPosition(worldPosition);
                transform->SetLocalRotation(worldRotation);
                transform->SetLocalScale(worldScale);
            }
        }
        else if (TransformComponent* transform = GetTransform(child))
        {
            transform->MarkDirty();
        }
        return true;
    }

    bool Scene::HasParent(Entity entity) const
    {
        return IsValid(entity) && m_Parents.contains(entity.Index);
    }

    Entity Scene::GetParent(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return {};
        }
        const auto it = m_Parents.find(entity.Index);
        return it != m_Parents.end() ? it->second : Entity {};
    }

    std::span<const Entity> Scene::GetChildren(Entity entity) const
    {
        if (!IsValid(entity))
        {
            return std::span<const Entity>(EmptyChildren);
        }
        const auto it = m_Children.find(entity.Index);
        return it != m_Children.end() ? std::span<const Entity>(it->second) : std::span<const Entity>(EmptyChildren);
    }

    std::span<const Entity> Scene::GetRootEntities() const
    {
        return m_RootEntities;
    }

    bool Scene::IsDescendantOf(Entity entity, Entity possibleAncestor) const
    {
        if (!IsValid(entity) || !IsValid(possibleAncestor))
        {
            return false;
        }

        for (Entity parent = GetParent(entity); parent.IsValid(); parent = GetParent(parent))
        {
            if (parent == possibleAncestor)
            {
                return true;
            }
        }
        return false;
    }

    void Scene::SetWorldPosition(Entity entity, const Vec3& position)
    {
        TransformComponent* transform = GetTransform(entity);
        if (transform == nullptr)
        {
            return;
        }

        UpdateTransforms();
        if (const TransformComponent* parent = FindInheritedParentTransform(*this, entity))
        {
            transform->SetLocalPosition(
                Math::TransformPoint(Math::Inverse(parent->GetWorldMatrix()), position));
        }
        else
        {
            transform->SetLocalPosition(position);
        }
    }

    void Scene::SetWorldRotation(Entity entity, const Quat& rotation)
    {
        TransformComponent* transform = GetTransform(entity);
        if (transform == nullptr)
        {
            return;
        }

        UpdateTransforms();
        if (const TransformComponent* parent = FindInheritedParentTransform(*this, entity))
        {
            transform->SetLocalRotation(Math::Normalize(
                Math::Inverse(parent->GetWorldRotation()) * rotation));
        }
        else
        {
            transform->SetLocalRotation(rotation);
        }
    }

    void Scene::SetWorldRotationDegrees(Entity entity, const Math::Rotator& rotation)
    {
        SetWorldRotation(entity, Math::ToQuat(rotation));
    }

    void Scene::SetWorldScale(Entity entity, const Vec3& scale)
    {
        TransformComponent* transform = GetTransform(entity);
        if (transform == nullptr)
        {
            return;
        }

        UpdateTransforms();
        if (const TransformComponent* parent = FindInheritedParentTransform(*this, entity))
        {
            // Ordinary TRS only; rotated non-uniform scale with shear is intentionally deferred.
            transform->SetLocalScale(SafeComponentDivide(scale, parent->GetWorldScale()));
        }
        else
        {
            transform->SetLocalScale(scale);
        }
    }

    void Scene::SetLocalPosition(Entity entity, const Vec3& position)
    {
        if (TransformComponent* transform = GetTransform(entity))
        {
            transform->SetLocalPosition(position);
        }
    }

    void Scene::SetLocalRotation(Entity entity, const Quat& rotation)
    {
        if (TransformComponent* transform = GetTransform(entity))
        {
            transform->SetLocalRotation(rotation);
        }
    }

    void Scene::SetLocalRotationDegrees(Entity entity, const Math::Rotator& rotation)
    {
        SetLocalRotation(entity, Math::ToQuat(rotation));
    }

    void Scene::SetLocalScale(Entity entity, const Vec3& scale)
    {
        if (TransformComponent* transform = GetTransform(entity))
        {
            transform->SetLocalScale(scale);
        }
    }

    const std::vector<Entity>& Scene::GetEntities() const
    {
        return m_Entities;
    }

    void Scene::UpdateTransforms()
    {
        TransformSystem system;
        system.Update(*this);
    }
}
