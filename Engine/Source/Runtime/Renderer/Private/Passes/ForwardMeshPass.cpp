#include "ForwardMeshPass.h"

#include "../Mesh/StaticMesh.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHICommandList.h>

namespace XEngine
{
    struct MeshPushConstants
    {
        Matrix4 ModelViewProjection;
    };

    void AddForwardMeshPass(
        RenderGraph& graph,
        RHIPipeline* pipeline,
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
            [pipeline, &objects](RenderGraphContext& context)
            {
                RHICommandList* commandList = context.GetCommandList();
                if (commandList == nullptr || pipeline == nullptr)
                {
                    return;
                }

                commandList->SetGraphicsPipeline(pipeline);

                for (const RenderObject& object : objects)
                {
                    if (object.Mesh == nullptr || !object.Mesh->VertexBuffer || !object.Mesh->IndexBuffer)
                    {
                        continue;
                    }

                    commandList->SetVertexBuffer(object.Mesh->VertexBuffer.get());
                    commandList->SetIndexBuffer(object.Mesh->IndexBuffer.get(), object.Mesh->IndexFormat);

                    MeshPushConstants constants;
                    constants.ModelViewProjection = object.ModelViewProjection;
                    commandList->PushConstants(ShaderStage::Vertex, &constants, sizeof(constants));

                    commandList->DrawIndexed(object.Mesh->IndexCount, 1, 0, 0, 0);
                }
            });
    }
}
