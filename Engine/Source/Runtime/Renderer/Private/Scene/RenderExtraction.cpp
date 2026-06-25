#include "RenderExtraction.h"

#include "../Resources/RenderResourceContext.h"
#include "../Resources/RenderMaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderTextureManager.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Math/Math.h>
#include <XEngine/Scene/Scene.h>

namespace XEngine
{
    static RenderLightType ConvertLightType(const LightType& type)
    {
        switch (type)
        {
        case LightType::Directional:
            return RenderLightType::Directional;
        case LightType::Point:
            return RenderLightType::Point;
        case LightType::Spot:
        default:
            return RenderLightType::Spot;
        }
    }

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

            if (transform == nullptr)
                continue;

            // Extract render object with mesh and material
            if ( renderer != nullptr && renderer->Visible)
            {
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
                object.WorldMatrix = transform->GetWorldMatrix();
                object.PreviousWorldMatrix = transform->GetPreviousWorldMatrix();
                object.Mesh = mesh;
                object.Material = material;
                object.WorldBounds = Math::TransformAABB(
                    meshAsset->Bounds,
                    transform->GetWorldMatrix());
                object.ObjectId = entity.Index + 1u;

                object.Visible       = renderer->Visible;
                object.CastShadow    = renderer->CastShadow;
                object.ReceiveShadow = renderer->ReceiveShadow;

                outRenderScene.OpaqueObjects.push_back(object);
            }

            // Extract scene light
            const LightComponent* light = scene.GetLight(entity);
            if (light != nullptr && light->Enabled)
            {
                RenderLight renderLight {};
                renderLight.Type = ConvertLightType(light->Type);
                renderLight.Position = transform->GetWorldPosition();
                renderLight.Range = light->Range;

                renderLight.Color = light->Color;
                renderLight.Intensity = light->Intensity;
                renderLight.InnerConeAngleRadians = Math::Radians(light->InnerConeAngleDegree);
                renderLight.OuterConeAngleRadians = Math::Radians(light->OuterConeAngleDegree);
                renderLight.CastShadow = light->CastShadow;

                const Vec3 forward = Math::GetForwardVector(transform->GetWorldRotation());
                renderLight.DirectionToLight = Math::Normalize(-forward);

                outRenderScene.Lights.push_back(renderLight);
            }
        }
    }
}
