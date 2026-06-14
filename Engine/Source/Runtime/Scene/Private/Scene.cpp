#include <XEngine/Scene/Scene.h>

#include <XEngine/Core/Assert.h>
#include <XEngine/Math/MathFunctions.h>

#include <algorithm>
#include <utility>

namespace XEngine
{
    void TransformComponent::UpdateMatrices()
    {
        PreviousWorldMatrix = WorldMatrix;

        if (!Dirty)
        {
            return;
        }

        LocalMatrix = Translate(Position) * Rotate(Rotation) * XEngine::Scale(Scale);

        // TODO later stage: add parent/child hierarchy and transform propagation.
        WorldMatrix = LocalMatrix;
        Dirty = false;
    }

    Scene::Scene() = default;
    Scene::~Scene() = default;

    Entity Scene::CreateEntity(const std::string& name)
    {
        EntityRecord record;
        record.Generation = 1;
        record.Alive = true;
        record.Name = name;

        Entity entity;
        entity.Index = static_cast<u32>(m_EntityRecords.size());
        entity.Generation = record.Generation;

        m_EntityRecords.push_back(std::move(record));
        m_Entities.push_back(entity);
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!IsValid(entity))
        {
            return;
        }

        EntityRecord& record = m_EntityRecords[entity.Index];
        record.Alive = false;
        ++record.Generation;
        record.Name.clear();

        m_Transforms.erase(entity.Index);
        m_MeshRenderers.erase(entity.Index);
        m_Cameras.erase(entity.Index);

        m_Entities.erase(
            std::remove(m_Entities.begin(), m_Entities.end(), entity),
            m_Entities.end());
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

    TransformComponent& Scene::AddTransform(Entity entity)
    {
        XENGINE_ASSERT(IsValid(entity), "Cannot add TransformComponent to invalid entity");
        TransformComponent& transform = m_Transforms[entity.Index];
        transform.Dirty = true;
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

    const std::vector<Entity>& Scene::GetEntities() const
    {
        return m_Entities;
    }

    void Scene::UpdateTransforms()
    {
        for (auto& [entityIndex, transform] : m_Transforms)
        {
            (void)entityIndex;
            transform.UpdateMatrices();
        }
    }
}
