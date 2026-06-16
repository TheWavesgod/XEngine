#include "RendererDebugPanel.h"

#include <imgui.h>

namespace XEngine
{
    void RendererDebugPanel::Draw(EditorContext& context)
    {
        if (!context.ShowRendererDebug)
        {
            return;
        }

        ImGui::Begin("Renderer Debug", &context.ShowRendererDebug);

        if (ImGui::Checkbox("Use Editor Camera", &context.UseEditorCamera) &&
            !context.UseEditorCamera)
        {
            context.ViewportInputMode = ViewportInputMode::UI;
        }

        if (context.RendererDebug == nullptr)
        {
            ImGui::TextUnformatted("Renderer debug settings unavailable");
            ImGui::End();
            return;
        }

        ImGui::Separator();
        ImGui::Checkbox("Visualize Lighting", &context.RendererDebug->VisualizeLighting);
        ImGui::Checkbox("Visualize Normals", &context.RendererDebug->VisualizeNormals);
        ImGui::Checkbox("Visualize Base Color", &context.RendererDebug->VisualizeBaseColor);
        ImGui::Checkbox("Visualize Cascades", &context.RendererDebug->VisualizeCascades);
        ImGui::Checkbox("Freeze Shadow Matrices", &context.RendererDebug->FreezeShadowMatrices);
        ImGui::TextWrapped("Shader debug output is stored here; renderer wiring for these modes is deferred until a pass consumes the settings.");

        ImGui::End();
    }
}
