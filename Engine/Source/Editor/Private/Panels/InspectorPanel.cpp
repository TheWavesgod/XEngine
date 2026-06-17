#include "InspectorPanel.h"

#include <XEngine/Asset/AssetMetadata.h>
#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Math/MathFunctions.h>
#include <XEngine/Scene/Components/CameraComponent.h>
#include <XEngine/Scene/Components/LightComponent.h>
#include <XEngine/Scene/Components/MeshRendererComponent.h>
#include <XEngine/Scene/Components/TransformComponent.h>
#include <XEngine/Scene/Scene.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <string>

namespace XEngine
{
    namespace
    {
        bool DragVec3(const char* label, Vec3& value, float speed = 0.05f)
        {
            float data[3] = { value.x, value.y, value.z };
            if (!ImGui::DragFloat3(label, data, speed))
            {
                return false;
            }

            value = Vec3 { data[0], data[1], data[2] };
            return true;
        }

        std::string AssetLabel(const EditorContext& context, AssetHandle handle)
        {
            if (context.Assets == nullptr || !handle.IsValid())
            {
                return "<none>";
            }

            if (const AssetMetadata* metadata = context.Assets->GetMetadata(handle))
            {
                return metadata->SourcePath.generic_string();
            }

            return "<unresolved>";
        }
    }

    void InspectorPanel::Draw(EditorContext& context)
    {
        if (!context.ShowInspector)
        {
            return;
        }

        ImGui::Begin("Inspector", &context.ShowInspector);

        Scene* scene = context.ActiveScene;
        if (scene == nullptr || !context.SelectedEntity.IsValid() || !scene->IsValid(context.SelectedEntity))
        {
            ImGui::TextUnformatted("No entity selected");
            context.SelectedEntity = {};
            ImGui::End();
            return;
        }

        const Entity entity = context.SelectedEntity;

        std::string name = scene->GetEntityName(entity);
        if (ImGui::InputText("Name", &name))
        {
            scene->SetEntityName(entity, name);
            context.SceneDirty = true;
        }

        if (TransformComponent* transform = scene->GetTransform(entity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const bool hasParent = scene->HasParent(entity);
                if (!hasParent)
                {
                    context.TransformSpace = TransformEditSpace::World;
                }

                if (hasParent)
                {
                    int editSpace = context.TransformSpace == TransformEditSpace::World ? 0 : 1;
                    const char* spaces[] = { "World", "Local" };
                    if (ImGui::Combo("Space", &editSpace, spaces, 2))
                    {
                        context.TransformSpace = editSpace == 0 ? TransformEditSpace::World : TransformEditSpace::Local;
                    }
                }
                else
                {
                    ImGui::TextUnformatted("Space: World");
                }

                const bool editingWorld = context.TransformSpace == TransformEditSpace::World;
                Vec3 position = editingWorld ? transform->GetWorldPosition() : transform->GetLocalPosition();
                Math::Rotator rotation = editingWorld ?
                    transform->GetWorldRotationDegrees() :
                    transform->GetLocalRotationDegrees();
                Vec3 scale = editingWorld ? transform->GetWorldScale() : transform->GetLocalScale();

                bool changed = false;
                changed |= DragVec3("Position", position);

                float rotationDegrees[3] = {
                    rotation.Roll,
                    rotation.Pitch,
                    rotation.Yaw
                };
                if (ImGui::DragFloat3("Rotation", rotationDegrees, 0.25f))
                {
                    rotation = Math::Rotator {
                        rotationDegrees[0],
                        rotationDegrees[1],
                        rotationDegrees[2]
                    };
                    changed = true;
                }

                changed |= DragVec3("Scale", scale);

                if (changed)
                {
                    // Editor transform editing defaults to World. Local editing
                    // is editor state only and is available for child entities.
                    if (editingWorld)
                    {
                        scene->SetWorldPosition(entity, position);
                        scene->SetWorldRotationDegrees(entity, rotation);
                        scene->SetWorldScale(entity, scale);
                    }
                    else
                    {
                        scene->SetLocalPosition(entity, position);
                        scene->SetLocalRotationDegrees(entity, rotation);
                        scene->SetLocalScale(entity, scale);
                    }
                    scene->UpdateTransforms();
                    context.SceneDirty = true;
                }

                const Vec3& localPosition = transform->GetLocalPosition();
                const Vec3& worldPosition = transform->GetWorldPosition();
                ImGui::Text(
                    "Local Position: %.3f, %.3f, %.3f",
                    localPosition.x,
                    localPosition.y,
                    localPosition.z);
                ImGui::Text(
                    "World Position: %.3f, %.3f, %.3f",
                    worldPosition.x,
                    worldPosition.y,
                    worldPosition.z);
            }
        }
        else
        {
            ImGui::TextUnformatted("Selected entity has no TransformComponent");
        }

        if (LightComponent* light = scene->GetLight(entity))
        {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool changed = false;
                int type = static_cast<int>(light->Type);
                const char* types[] = { "Directional", "Point", "Spot" };
                if (ImGui::Combo("Type", &type, types, 3))
                {
                    light->Type = static_cast<LightType>(type);
                    changed = true;
                }

                float color[3] = { light->Color.x, light->Color.y, light->Color.z };
                if (ImGui::ColorEdit3("Color", color))
                {
                    light->Color = Vec3 { color[0], color[1], color[2] };
                    changed = true;
                }

                changed |= ImGui::DragFloat("Intensity", &light->Intensity, 0.05f, 0.0f, 1000.0f);
                changed |= ImGui::DragFloat("Range", &light->Range, 0.05f, 0.0f, 10000.0f);
                changed |= ImGui::DragFloat(
                    "Inner Cone Angle Degrees",
                    &light->InnerConeAngleDegree,
                    0.25f,
                    0.0f,
                    180.0f);
                changed |= ImGui::DragFloat(
                    "Outer Cone Angle Degrees",
                    &light->OuterConeAngleDegree,
                    0.25f,
                    0.0f,
                    180.0f);
                // CastShadow is serialized now for Stage 9 CSM even though this
                // stage does not render shadow maps.
                changed |= ImGui::Checkbox("Cast Shadow", &light->CastShadow);
                changed |= ImGui::Checkbox("Enabled", &light->Enabled);

                if (changed)
                {
                    context.SceneDirty = true;
                }
            }
        }

        if (CameraComponent* camera = scene->GetCamera(entity))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool changed = false;
                float fovDegrees = Math::Degrees(camera->VerticalFovRadians);
                if (ImGui::DragFloat("FOV degrees", &fovDegrees, 0.25f, 1.0f, 179.0f))
                {
                    camera->VerticalFovRadians = Math::Radians(fovDegrees);
                    changed = true;
                }
                changed |= ImGui::DragFloat("Near plane", &camera->NearPlane, 0.01f, 0.001f, camera->FarPlane);
                changed |= ImGui::DragFloat("Far plane", &camera->FarPlane, 1.0f, camera->NearPlane, 100000.0f);
                if (ImGui::Checkbox("Primary", &camera->Primary))
                {
                    if (camera->Primary)
                    {
                        for (Entity other : scene->GetEntities())
                        {
                            if (other != entity)
                            {
                                if (CameraComponent* otherCamera = scene->GetCamera(other))
                                {
                                    otherCamera->Primary = false;
                                }
                            }
                        }
                    }
                    changed = true;
                }

                if (changed)
                {
                    context.SceneDirty = true;
                }
            }
        }

        if (MeshRendererComponent* renderer = scene->GetMeshRenderer(entity))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const std::string meshLabel = AssetLabel(context, renderer->MeshAsset);
                const std::string materialLabel = AssetLabel(context, renderer->MaterialAsset);
                ImGui::Text("Mesh asset reference/path: %s", meshLabel.c_str());
                ImGui::Text("Material asset reference/path: %s", materialLabel.c_str());
                bool changed = false;
                changed |= ImGui::Checkbox("Visible", &renderer->Visible);
                changed |= ImGui::Checkbox("Cast Shadow", &renderer->CastShadow);
                changed |= ImGui::Checkbox("Receive Shadow", &renderer->ReceiveShadow);
                if (changed)
                {
                    context.SceneDirty = true;
                }
            }
        }

        ImGui::End();
    }
}
