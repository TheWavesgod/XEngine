#include "TransformSystem.h"

#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Scene.h>

namespace XEngine
{
    void TransformSystem::Update(Scene& scene)
    {
        for (Entity root : scene.GetRootEntities())
        {
            UpdateRecursive(scene, root, nullptr);
        }
    }

    void TransformSystem::UpdateRecursive(
        Scene& scene,
        Entity entity,
        const TransformComponent* parentTransform)
    {
        TransformComponent* transform = scene.GetTransform(entity);
        const TransformComponent* childParentTransform = parentTransform;

        if (transform != nullptr)
        {
            transform->m_PreviousWorldMatrix = transform->m_WorldMatrix;
            transform->m_LocalMatrix = Math::ComposeTRS(
                transform->m_LocalPosition,
                transform->m_LocalRotation,
                transform->m_LocalScale);

            if (parentTransform != nullptr)
            {
                transform->m_WorldMatrix =
                    parentTransform->m_WorldMatrix * transform->m_LocalMatrix;
                transform->m_WorldPosition =
                    Math::ExtractTranslation(transform->m_WorldMatrix);
                transform->m_WorldRotation = Math::Normalize(
                    parentTransform->m_WorldRotation * transform->m_LocalRotation);
                transform->m_WorldScale =
                    parentTransform->m_WorldScale * transform->m_LocalScale;
            }
            else
            {
                transform->m_WorldMatrix = transform->m_LocalMatrix;
                transform->m_WorldPosition = transform->m_LocalPosition;
                transform->m_WorldRotation = transform->m_LocalRotation;
                transform->m_WorldScale = transform->m_LocalScale;
            }

            transform->ClearDirty();
            childParentTransform = transform;
        }

        for (Entity child : scene.GetChildren(entity))
        {
            UpdateRecursive(scene, child, childParentTransform);
        }
    }
}
