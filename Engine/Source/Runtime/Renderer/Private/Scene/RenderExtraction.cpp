#include "RenderExtraction.h"

#include "../Resources/RenderResourceContext.h"
#include "../Resources/RenderMaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderTextureManager.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Math/AABB.h>
#include <XEngine/Scene/Scene.h>

namespace XEngine
{
    void RenderExtraction::Extract(
        const Scene& scene,
        AssetSystem& assetSystem,
        RenderResourceContext& resources,
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

            if (!resources.IsValid())
            {
                continue;
            }

            const MeshHandle mesh = resources.Meshes->GetOrCreateMeshFromAsset(renderer->MeshAsset, *meshAsset);
            const MaterialHandle material = resources.Materials->GetOrCreateMaterialFromAsset(
                renderer->MaterialAsset,
                *materialAsset,
                assetSystem,
                *resources.Textures);
                
            if (!mesh.IsValid() || !material.IsValid())
            {
                continue;
            }

            RenderObject object;
            object.WorldMatrix = transform->WorldMatrix;
            object.PreviousWorldMatrix = transform->PreviousWorldMatrix;
            object.Mesh = mesh;
            object.Material = material;
            object.WorldBounds = TransformAABB(meshAsset->Bounds, transform->WorldMatrix);
            object.ObjectId = entity.Index + 1u;
            outRenderScene.OpaqueObjects.push_back(object);
        }
    }
}
