#include "RenderExtraction.h"

#include "../Materials/MaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/TextureManager.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Scene/Scene.h>

#include <glm/common.hpp>

#include <limits>

namespace XEngine
{
    namespace
    {
        AABB TransformBounds(const AABB& bounds, const Mat4& world)
        {
            const Vec3 corners[] = {
                { bounds.Min.x, bounds.Min.y, bounds.Min.z },
                { bounds.Max.x, bounds.Min.y, bounds.Min.z },
                { bounds.Min.x, bounds.Max.y, bounds.Min.z },
                { bounds.Max.x, bounds.Max.y, bounds.Min.z },
                { bounds.Min.x, bounds.Min.y, bounds.Max.z },
                { bounds.Max.x, bounds.Min.y, bounds.Max.z },
                { bounds.Min.x, bounds.Max.y, bounds.Max.z },
                { bounds.Max.x, bounds.Max.y, bounds.Max.z },
            };

            AABB transformed;
            transformed.Min = Vec3 { std::numeric_limits<float>::max() };
            transformed.Max = Vec3 { std::numeric_limits<float>::lowest() };

            for (const Vec3& corner : corners)
            {
                const Vec4 point = world * Vec4 { corner, 1.0f };
                transformed.Min = glm::min(transformed.Min, Vec3 { point });
                transformed.Max = glm::max(transformed.Max, Vec3 { point });
            }

            return transformed;
        }
    }

    void RenderExtraction::Extract(
        const Scene& scene,
        AssetSystem& assetSystem,
        RenderMeshManager& meshManager,
        MaterialSystem& materialSystem,
        TextureManager& textureManager,
        RenderScene& outRenderScene)
    {
        outRenderScene.Clear();

        for (Entity entity : scene.GetEntities())
        {
            const TransformComponent* transform = scene.GetTransform(entity);
            const MeshRendererComponent* renderer = scene.GetMeshRenderer(entity);
            if (transform == nullptr || renderer == nullptr || !renderer->Visible)
            {
                continue;
            }

            const MeshAsset* meshAsset = assetSystem.GetMeshAsset(renderer->MeshAsset);
            if (meshAsset == nullptr)
            {
                continue;
            }

            const MaterialAsset* materialAsset = assetSystem.GetMaterialAsset(renderer->MaterialAsset);
            if (materialAsset == nullptr)
            {
                continue;
            }

            const MeshHandle mesh = meshManager.GetOrCreateMeshFromAsset(renderer->MeshAsset, *meshAsset);
            const MaterialHandle material = materialSystem.GetOrCreateMaterialFromAsset(
                renderer->MaterialAsset,
                *materialAsset,
                assetSystem,
                textureManager);
            if (!mesh.IsValid() || !material.IsValid())
            {
                continue;
            }

            RenderObject object;
            object.WorldMatrix = transform->WorldMatrix;
            object.PreviousWorldMatrix = transform->PreviousWorldMatrix;
            object.Mesh = mesh;
            object.Material = material;
            object.WorldBounds = TransformBounds(meshAsset->Bounds, transform->WorldMatrix);
            object.ObjectId = entity.Index + 1u;
            outRenderScene.OpaqueObjects.push_back(object);
        }
    }
}
