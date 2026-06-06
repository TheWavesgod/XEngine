#include "ForwardOpaquePass.h"

#include "../Materials/MaterialSystem.h"
#include "../Mesh/StaticMesh.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/RHI/RHICommandList.h>

namespace XEngine
{
    struct PBRPushConstants
    {
        Matrix4 ModelViewProjection;
        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4 MaterialFactors { 0.0f, 1.0f, 0.5f, 0.0f };
    };

    void AddForwardOpaquePass(
        RenderGraph& graph,
        RHIPipeline* pbrPipeline,
        MaterialSystem* materialSystem,
        const std::vector<RenderObject>& objects)
    {
        RenderGraphPassDesc desc;
        desc.Name = "ForwardOpaquePass";
        desc.Type = RenderGraphPassType::Graphics;

        graph.AddPass(
            desc,
            [](RenderGraphBuilder&)
            {
                // TODO Stage 9:
                // Declare HDR color/depth graph resources when RenderGraph grows real resource tracking.
            },
            [pbrPipeline, materialSystem, &objects](RenderGraphContext& context)
            {
                RHICommandList* commandList = context.GetCommandList();
                if (commandList == nullptr || pbrPipeline == nullptr || materialSystem == nullptr)
                {
                    return;
                }

                commandList->SetGraphicsPipeline(pbrPipeline);

                for (const RenderObject& object : objects)
                {
                    if (object.Mesh == nullptr || !object.Mesh->VertexBuffer || !object.Mesh->IndexBuffer)
                    {
                        continue;
                    }

                    MaterialHandle material = object.Material;
                    const MaterialDesc* materialDesc = materialSystem->GetMaterialDesc(material);
                    const GPUMaterialData* gpuMaterial = materialSystem->GetGPUMaterialData(material);
                    if (materialDesc == nullptr || gpuMaterial == nullptr ||
                        materialDesc->ShadingModel != MaterialShadingModel::Lit)
                    {
                        material = materialSystem->GetDefaultLitMaterial();
                        materialDesc = materialSystem->GetMaterialDesc(material);
                        gpuMaterial = materialSystem->GetGPUMaterialData(material);
                    }

                    RHIBindGroup* bindGroup = materialSystem->GetPBRMaterialBindGroup(material);
                    if (bindGroup == nullptr)
                    {
                        bindGroup = materialSystem->GetPBRMaterialBindGroup(materialSystem->GetDefaultLitMaterial());
                    }

                    if (bindGroup == nullptr || materialDesc == nullptr || gpuMaterial == nullptr)
                    {
                        continue;
                    }

                    commandList->SetVertexBuffer(object.Mesh->VertexBuffer.get());
                    commandList->SetIndexBuffer(object.Mesh->IndexBuffer.get(), object.Mesh->IndexFormat);
                    commandList->SetBindGroup(0, bindGroup);

                    PBRPushConstants constants;
                    constants.ModelViewProjection = object.ModelViewProjection;
                    constants.BaseColorFactor = gpuMaterial->BaseColorFactor;
                    constants.MaterialFactors = Vec4 {
                        gpuMaterial->MetallicFactor,
                        gpuMaterial->RoughnessFactor,
                        gpuMaterial->AlphaCutoff,
                        0.0f
                    };
                    commandList->PushConstants(RHIShaderStageFlags::AllGraphics, &constants, sizeof(constants));

                    commandList->DrawIndexed(object.Mesh->IndexCount, 1, 0, 0, 0);
                }
            });
    }
}
