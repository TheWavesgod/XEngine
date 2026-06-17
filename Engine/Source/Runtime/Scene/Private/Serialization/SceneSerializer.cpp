#include <XEngine/Scene/SceneSerializer.h>

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Core/ProjectPaths.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/LightComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Scene.h>
#include <XEngine/Serialization/JsonSerialization.h>
#include <XEngine/Serialization/SerializationVersion.h>

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    namespace
    {
        using Json = JsonSerialization::Json;

        Vec3 ReadVec3(const Json& json, const Vec3& fallback = Vec3 { 0.0f })
        {
            if (!json.is_array() || json.size() != 3)
            {
                return fallback;
            }

            return Vec3 {
                json[0].get<float>(),
                json[1].get<float>(),
                json[2].get<float>()
            };
        }

        Json WriteVec3(const Vec3& value)
        {
            return Json::array({ value.x, value.y, value.z });
        }

        Math::Rotator ReadRotator(const Json& json)
        {
            if (!json.is_array() || json.size() != 3)
            {
                return {};
            }

            return Math::Rotator {
                json[0].get<float>(),
                json[1].get<float>(),
                json[2].get<float>()
            };
        }

        Json WriteRotator(const Math::Rotator& value)
        {
            return Json::array({ value.Roll, value.Pitch, value.Yaw });
        }

        const char* LightTypeToString(LightType type)
        {
            switch (type)
            {
            case LightType::Directional:
                return "Directional";
            case LightType::Point:
                return "Point";
            case LightType::Spot:
                return "Spot";
            default:
                return "Directional";
            }
        }

        LightType LightTypeFromString(const std::string& value)
        {
            if (value == "Point")
            {
                return LightType::Point;
            }
            if (value == "Spot")
            {
                return LightType::Spot;
            }
            return LightType::Directional;
        }

        const char* ProjectionModeToString(CameraProjectionMode mode)
        {
            return mode == CameraProjectionMode::Orthographic ? "Orthographic" : "Perspective";
        }

        CameraProjectionMode ProjectionModeFromString(const std::string& value)
        {
            return value == "Orthographic" ?
                CameraProjectionMode::Orthographic :
                CameraProjectionMode::Perspective;
        }

        bool FindFirstMeshAndMaterial(
            const AssetImportResult& importResult,
            const AssetSystem& assetSystem,
            AssetHandle& outMesh,
            AssetHandle& outMaterial)
        {
            for (AssetHandle handle : importResult.ImportedAssets)
            {
                if (!outMesh.IsValid() && assetSystem.GetMeshAsset(handle) != nullptr)
                {
                    outMesh = handle;
                }
                if (!outMaterial.IsValid() && assetSystem.GetMaterialAsset(handle) != nullptr)
                {
                    outMaterial = handle;
                }
            }
            return outMesh.IsValid() || outMaterial.IsValid();
        }

        AssetHandle ResolveMeshReference(AssetSystem* assetSystem, const std::string& reference)
        {
            if (assetSystem == nullptr || reference.empty())
            {
                return {};
            }

            if (reference == "procedural:cube")
            {
                return assetSystem->CreateProceduralCubeMeshAsset("SceneProceduralCube");
            }

            AssetHandle mesh;
            AssetHandle material;
            const AssetImportResult result = assetSystem->ImportAsset(reference);
            if (!result.Succeeded())
            {
                XENGINE_LOG_WARN(std::string("Missing asset reference: ") + reference);
                return {};
            }
            FindFirstMeshAndMaterial(result, *assetSystem, mesh, material);
            return mesh;
        }

        AssetHandle ResolveMaterialReference(
            AssetSystem* assetSystem,
            const std::string& reference,
            const std::string& meshReference)
        {
            if (assetSystem == nullptr)
            {
                return {};
            }

            if (reference == "procedural:test-material" || reference.empty())
            {
                return assetSystem->CreateTestMaterialAsset("SceneDefaultMaterial", {});
            }

            AssetHandle mesh;
            AssetHandle material;
            const AssetImportResult result = assetSystem->ImportAsset(reference);
            if (result.Succeeded())
            {
                FindFirstMeshAndMaterial(result, *assetSystem, mesh, material);
                if (material.IsValid())
                {
                    return material;
                }
            }

            if (!meshReference.empty() && meshReference != reference)
            {
                const AssetImportResult meshResult = assetSystem->ImportAsset(meshReference);
                if (meshResult.Succeeded())
                {
                    FindFirstMeshAndMaterial(meshResult, *assetSystem, mesh, material);
                    if (material.IsValid())
                    {
                        return material;
                    }
                }
            }

            XENGINE_LOG_WARN(std::string("Missing asset reference: ") + reference);
            return assetSystem->CreateTestMaterialAsset("SceneDefaultMaterial", {});
        }

        std::string ToAssetVirtualPath(const std::filesystem::path& path)
        {
            const std::filesystem::path normalizedPath = path.lexically_normal();
            const std::filesystem::path assetRoot = ProjectPaths::GetAssetRoot().lexically_normal();
            auto pathIt = normalizedPath.begin();
            auto rootIt = assetRoot.begin();
            for (; rootIt != assetRoot.end() && pathIt != normalizedPath.end(); ++rootIt, ++pathIt)
            {
                if (*rootIt != *pathIt)
                {
                    return normalizedPath.generic_string();
                }
            }

            if (rootIt != assetRoot.end())
            {
                return normalizedPath.generic_string();
            }

            std::filesystem::path relative;
            for (; pathIt != normalizedPath.end(); ++pathIt)
            {
                relative /= *pathIt;
            }
            return "asset://" + relative.generic_string();
        }

        std::string ResolveAssetPath(const AssetSystem* assetSystem, AssetHandle handle)
        {
            if (assetSystem == nullptr || !handle.IsValid())
            {
                return {};
            }

            if (const AssetMetadata* metadata = assetSystem->GetMetadata(handle))
            {
                const std::string sourcePath = metadata->SourcePath.generic_string();
                if (sourcePath.starts_with("asset://"))
                {
                    return sourcePath;
                }
                if (sourcePath.starts_with("procedural/"))
                {
                    if (metadata->Type == AssetType::Mesh)
                    {
                        return "procedural:cube";
                    }
                    if (metadata->Type == AssetType::Material)
                    {
                        return "procedural:test-material";
                    }
                    return sourcePath;
                }

                // Scene files persist portable virtual asset paths, never local
                // absolute development paths.
                return ToAssetVirtualPath(ProjectPaths::Resolve(sourcePath));
            }
            return {};
        }
    }

    SceneSerializer::SceneSerializer(const SerializationContext& context)
        : m_Context(context)
    {
    }

    bool SceneSerializer::LoadFromFile(Scene& scene, const std::filesystem::path& path)
    {
        Json root;
        const std::filesystem::path resolvedPath = ProjectPaths::Resolve(path.generic_string());
        if (!JsonSerialization::LoadJsonFile(resolvedPath, root))
        {
            return false;
        }

        scene.Clear();

        if (!root.contains("entities") || !root["entities"].is_array())
        {
            XENGINE_LOG_ERROR("Scene file is missing entities array");
            return false;
        }

        std::unordered_map<std::string, Entity> entitiesByName;
        std::unordered_map<std::string, Entity> entitiesById;
        std::vector<std::pair<Entity, std::string>> deferredParents;

        for (const Json& entityJson : root["entities"])
        {
            const std::string name = entityJson.value("name", "Entity");
            Entity entity = scene.CreateEntity(name);
            entitiesByName[name] = entity;
            entitiesById[entityJson.value("id", name)] = entity;

            if (entityJson.contains("Transform"))
            {
                const Json& transformJson = entityJson["Transform"];
                TransformComponent& transform = scene.AddTransform(entity);
                transform.SetLocalPosition(ReadVec3(transformJson.value("position", Json::array({ 0.0f, 0.0f, 0.0f }))));
                transform.SetLocalRotationDegrees(ReadRotator(transformJson.value("rotationDegrees", Json::array({ 0.0f, 0.0f, 0.0f }))));
                transform.SetLocalScale(ReadVec3(transformJson.value("scale", Json::array({ 1.0f, 1.0f, 1.0f })), Vec3 { 1.0f }));
            }

            if (entityJson.contains("Camera"))
            {
                const Json& cameraJson = entityJson["Camera"];
                CameraComponent& camera = scene.AddCamera(entity);
                camera.ProjectionMode = ProjectionModeFromString(cameraJson.value("projection", "Perspective"));
                camera.VerticalFovRadians = Math::Radians(cameraJson.value("verticalFovDegrees", 60.0f));
                camera.NearPlane = cameraJson.value("nearPlane", 0.1f);
                camera.FarPlane = cameraJson.value("farPlane", 1000.0f);
                camera.OrthographicHeight = cameraJson.value("orthographicHeight", 10.0f);
                camera.Primary = cameraJson.value("primary", false);
            }

            if (entityJson.contains("Light"))
            {
                const Json& lightJson = entityJson["Light"];
                LightComponent& light = scene.AddLight(entity);
                light.Type = LightTypeFromString(lightJson.value("type", "Directional"));
                light.Color = ReadVec3(lightJson.value("color", Json::array({ 1.0f, 1.0f, 1.0f })), Vec3 { 1.0f });
                light.Intensity = lightJson.value("intensity", 1.0f);
                light.Range = lightJson.value("range", 10.0f);
                light.InnerConeAngleDegree = lightJson.value("innerConeDegrees", 30.0f);
                light.OuterConeAngleDegree = lightJson.value("outerConeDegrees", 60.0f);
                light.CastShadow = lightJson.value("castShadow", true);
                light.Enabled = lightJson.value("enabled", true);
            }

            if (entityJson.contains("MeshRenderer"))
            {
                const Json& rendererJson = entityJson["MeshRenderer"];
                MeshRendererComponent& renderer = scene.AddMeshRenderer(entity);
                const std::string meshReference = rendererJson.value("mesh", "");
                const std::string materialReference = rendererJson.value("material", "");
                renderer.MeshAsset = ResolveMeshReference(m_Context.Assets, meshReference);
                renderer.MaterialAsset = ResolveMaterialReference(m_Context.Assets, materialReference, meshReference);
                renderer.Visible = rendererJson.value("visible", true);
                renderer.CastShadow = rendererJson.value("castShadow", true);
                renderer.ReceiveShadow = rendererJson.value("receiveShadow", true);

                if (!renderer.MeshAsset.IsValid())
                {
                    XENGINE_LOG_WARN(std::string("MeshRenderer missing valid mesh asset on entity: ") + name);
                }
                if (!renderer.MaterialAsset.IsValid())
                {
                    XENGINE_LOG_WARN(std::string("MeshRenderer missing valid material asset on entity: ") + name);
                }
            }

            for (auto it = entityJson.begin(); it != entityJson.end(); ++it)
            {
                const std::string key = it.key();
                if (key != "name" && key != "parent" && key != "Transform" &&
                    key != "Camera" && key != "Light" && key != "MeshRenderer")
                {
                    XENGINE_LOG_WARN(std::string("Unknown component type in scene file: ") + key);
                }
            }

            if (entityJson.contains("parent") && !entityJson["parent"].is_null())
            {
                deferredParents.push_back({ entity, entityJson.value("parent", "") });
            }
        }

        for (const auto& [child, parentName] : deferredParents)
        {
            auto parentIt = entitiesById.find(parentName);
            if (parentIt == entitiesById.end())
            {
                parentIt = entitiesByName.find(parentName);
            }
            if (parentIt == entitiesById.end() && parentIt == entitiesByName.end())
            {
                XENGINE_LOG_WARN(std::string("Missing parent reference: ") + parentName);
                continue;
            }
            if (!scene.SetParent(child, parentIt->second, false))
            {
                XENGINE_LOG_WARN(std::string("Rejected invalid or cyclic parent reference: ") + parentName);
            }
        }

        scene.UpdateTransforms();

        XENGINE_LOG_INFO(
            std::string("Scene entity count after load: ") +
            std::to_string(scene.GetEntities().size()));
        return true;
    }

    bool SceneSerializer::SaveToFile(const Scene& scene, const std::filesystem::path& path)
    {
        Json root;
        root["version"] = XSceneSerializationVersion;
        root["entities"] = Json::array();
        std::unordered_map<u32, std::string> serializedIds;
        for (Entity entity : scene.GetEntities())
        {
            serializedIds[entity.Index] = "entity_" + std::to_string(serializedIds.size());
        }

        for (Entity entity : scene.GetEntities())
        {
            Json entityJson;
            entityJson["id"] = serializedIds[entity.Index];
            entityJson["name"] = scene.GetEntityName(entity);

            // Root entities are children of the implicit SceneRoot and serialize
            // with parent=null; the UI-only root itself is never serialized.
            if (scene.HasParent(entity))
            {
                entityJson["parent"] = serializedIds[scene.GetParent(entity).Index];
            }
            else
            {
                entityJson["parent"] = nullptr;
            }

            if (const TransformComponent* transform = scene.GetTransform(entity))
            {
                Json transformJson;
                transformJson["position"] = WriteVec3(transform->GetLocalPosition());
                transformJson["rotationDegrees"] = WriteRotator(transform->GetLocalRotationDegrees());
                transformJson["scale"] = WriteVec3(transform->GetLocalScale());
                entityJson["Transform"] = transformJson;
            }

            if (const CameraComponent* camera = scene.GetCamera(entity))
            {
                Json cameraJson;
                cameraJson["projection"] = ProjectionModeToString(camera->ProjectionMode);
                cameraJson["verticalFovDegrees"] = Math::Degrees(camera->VerticalFovRadians);
                cameraJson["nearPlane"] = camera->NearPlane;
                cameraJson["farPlane"] = camera->FarPlane;
                cameraJson["orthographicHeight"] = camera->OrthographicHeight;
                cameraJson["primary"] = camera->Primary;
                entityJson["Camera"] = cameraJson;
            }

            if (const LightComponent* light = scene.GetLight(entity))
            {
                Json lightJson;
                lightJson["type"] = LightTypeToString(light->Type);
                lightJson["color"] = WriteVec3(light->Color);
                lightJson["intensity"] = light->Intensity;
                lightJson["range"] = light->Range;
                lightJson["innerConeDegrees"] = light->InnerConeAngleDegree;
                lightJson["outerConeDegrees"] = light->OuterConeAngleDegree;
                lightJson["castShadow"] = light->CastShadow;
                lightJson["enabled"] = light->Enabled;
                entityJson["Light"] = lightJson;
            }

            if (const MeshRendererComponent* renderer = scene.GetMeshRenderer(entity))
            {
                Json rendererJson;
                rendererJson["mesh"] = ResolveAssetPath(m_Context.Assets, renderer->MeshAsset);
                rendererJson["material"] = ResolveAssetPath(m_Context.Assets, renderer->MaterialAsset);
                rendererJson["visible"] = renderer->Visible;
                rendererJson["castShadow"] = renderer->CastShadow;
                rendererJson["receiveShadow"] = renderer->ReceiveShadow;
                entityJson["MeshRenderer"] = rendererJson;
            }

            root["entities"].push_back(entityJson);
        }

        return JsonSerialization::SaveJsonFile(ProjectPaths::Resolve(path.generic_string()), root);
    }
}
