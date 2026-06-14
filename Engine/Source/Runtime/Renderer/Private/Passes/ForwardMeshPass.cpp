#include "ForwardMeshPass.h"

#include "../Resources/RenderMaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderShaderTypes.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHICommandList.h>

namespace XEngine
{
    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
        RenderMaterialSystem* materialSystem,
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
                    constants.ModelViewProjection = object.WorldMatrix;
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
