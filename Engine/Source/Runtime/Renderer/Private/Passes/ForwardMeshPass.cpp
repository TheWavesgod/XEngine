#include "ForwardMeshPass.h"

#include "../Materials/MaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHICommandList.h>

namespace XEngine
{
    namespace
    {
        Matrix4 ToMatrix4(const Mat4& matrix)
        {
            Matrix4 result {};
            const float* values = &matrix[0][0];
            for (u32 i = 0; i < 16; ++i)
            {
                result.Values[i] = values[i];
            }
            return result;
        }
    }

    struct MeshPushConstants
    {
        Matrix4 ModelViewProjection;
    };

    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
        MaterialSystem* materialSystem,
        RenderMeshManager* meshManager,
        const std::vector<RenderObject>& objects)
    {
        RenderGraphPassDesc desc;
        desc.Name = "ForwardMeshPass";
        desc.Type = RenderGraphPassType::Graphics;

        graph.AddPass(
            desc,
            [](RenderGraphBuilder&)
            {
                // TODO Stage 6+: declare explicit graph buffer/texture accesses.
            },
            [pipeline, materialSystem, meshManager, &objects](RenderGraphContext& context)
            {
                RHICommandList* commandList = context.GetCommandList();
                if (commandList == nullptr || pipeline == nullptr || materialSystem == nullptr || meshManager == nullptr)
                {
                    return;
                }

                commandList->SetGraphicsPipeline(pipeline);

                for (const RenderObject& object : objects)
                {
                    const RenderMesh* mesh = meshManager->GetMesh(object.Mesh);
                    if (mesh == nullptr || !mesh->VertexBuffer || !mesh->IndexBuffer)
                    {
                        continue;
                    }

                    commandList->SetVertexBuffer(mesh->VertexBuffer.get());
                    commandList->SetIndexBuffer(mesh->IndexBuffer.get(), mesh->IndexFormat);

                    RHIBindGroup* bindGroup = materialSystem->GetBaseColorBindGroup(object.Material);
                    if (bindGroup == nullptr)
                    {
                        bindGroup = materialSystem->GetBaseColorBindGroup(materialSystem->GetDefaultUnlitMaterial());
                    }
                    commandList->SetBindGroup(0, bindGroup);

                    MeshPushConstants constants;
                    constants.ModelViewProjection = ToMatrix4(object.WorldMatrix);
                    commandList->PushConstants(RHIShaderStageFlags::Vertex, &constants, sizeof(constants));

                    for (const RenderSubmesh& submesh : mesh->Submeshes)
                    {
                        commandList->DrawIndexed(
                            submesh.IndexCount,
                            1,
                            submesh.FirstIndex,
                            submesh.VertexOffset,
                            0);
                    }
                }
            });
    }
}
