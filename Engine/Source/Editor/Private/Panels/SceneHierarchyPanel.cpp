#include "SceneHierarchyPanel.h"

#include <XEngine/Scene/Scene.h>

#include <imgui.h>

namespace XEngine
{
    void SceneHierarchyPanel::Draw(EditorContext& context)
    {
        if (!context.ShowSceneHierarchy)
        {
            return;
        }

        ImGui::Begin("Scene Hierarchy", &context.ShowSceneHierarchy);

        Scene* scene = context.ActiveScene;
        if (scene == nullptr || scene->GetEntities().empty())
        {
            ImGui::TextUnformatted("Empty scene");
            context.SelectedEntity = {};
            ImGui::End();
            return;
        }

        if (context.SelectedEntity.IsValid() && !scene->IsValid(context.SelectedEntity))
        {
            context.SelectedEntity = {};
        }

        const std::span<const Entity> roots = scene->GetRootEntities();
        if (roots.empty())
        {
            ImGui::TextUnformatted("No root entities");
        }

        const bool rootOpen = ImGui::TreeNodeEx(
            "SceneRoot",
            ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanAvailWidth,
            "Scene");
        if (ImGui::IsItemClicked())
        {
            context.SelectedEntity = {};
        }

        if (rootOpen)
        {
            for (Entity entity : roots)
            {
                DrawEntityNode(context, entity);
            }
            ImGui::TreePop();
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(EditorContext& context, Entity entity)
    {
        Scene* scene = context.ActiveScene;
        if (scene == nullptr || !scene->IsValid(entity))
        {
            return;
        }

        const std::span<const Entity> children = scene->GetChildren(entity);
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (children.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (context.SelectedEntity == entity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const std::string& name = scene->GetEntityName(entity);
        const bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(static_cast<uintptr_t>(entity.Index)),
            flags,
            "%s",
            name.empty() ? "Entity" : name.c_str());

        if (ImGui::IsItemClicked())
        {
            context.SelectedEntity = entity;
        }

        if (!children.empty() && open)
        {
            // Hierarchy traversal uses Scene query APIs only; the editor never
            // reaches into Scene's internal parent/child containers.
            for (Entity child : children)
            {
                DrawEntityNode(context, child);
            }
            ImGui::TreePop();
        }
    }
}
