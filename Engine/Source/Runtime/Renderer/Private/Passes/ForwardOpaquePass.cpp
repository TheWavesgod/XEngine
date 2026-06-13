#include "ForwardOpaquePass.h"

#include "../Resources/RenderResourceContext.h"
#include "../Materials/MaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"
#include "../Pipeline/RenderFrameContext.h"

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

    struct PBRPushConstants
    {
        Matrix4 ModelViewProjection;
        Vec4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4 MaterialFactors { 0.0f, 1.0f, 0.5f, 0.0f };
    };

    void AddForwardOpaquePass(
        RenderGraph& graph,
        const RenderFrameContext& frameContext,
        const RenderScene& renderScene,
        RenderResourceContext& resources)
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
            [&frameContext, &renderScene, &resources](RenderGraphContext& context)
            {
                RHICommandList* commandList = context.GetCommandList();
                if (commandList == nullptr || pbrPipeline == nullptr || !resources.IsValid())
                {
                    return;
                }

                commandList->SetGraphicsPipeline(pbrPipeline);

                for (const RenderObject& object : renderScene.OpaqueObjects)
                {
                    const RenderMesh* mesh = resources.Meshes->GetMesh(object.Mesh);
                    if (mesh == nullptr || !mesh->VertexBuffer || !mesh->IndexBuffer)
                    {
                        continue;
                    }

                    MaterialHandle material = object.Material;
                    const MaterialDesc* materialDesc = resources.Materials->GetMaterialDesc(material);
                    const GPUMaterialData* gpuMaterial = resources.Materials->GetGPUMaterialData(material);
                    if (materialDesc == nullptr || gpuMaterial == nullptr ||
                        materialDesc->ShadingModel != MaterialShadingModel::Lit)
                    {
                        material = resources.Materials->GetDefaultLitMaterial();
                        materialDesc = resources.Materials->GetMaterialDesc(material);
                        gpuMaterial = resources.Materials->GetGPUMaterialData(material);
                    }

                    RHIBindGroup* bindGroup = resources.Materials->GetPBRMaterialBindGroup(material);
                    if (bindGroup == nullptr)
                    {
                        bindGroup = resources.Materials->GetPBRMaterialBindGroup(resources.Materials->GetDefaultLitMaterial());
                    }

                    if (bindGroup == nullptr || materialDesc == nullptr || gpuMaterial == nullptr)
                    {
                        continue;
                    }

                    commandList->SetVertexBuffer(mesh->VertexBuffer.get());
                    commandList->SetIndexBuffer(mesh->IndexBuffer.get(), mesh->IndexFormat);
                    commandList->SetBindGroup(0, bindGroup);

                    const Mat4& viewProjection = frameContext.ViewProjectionMatrix;
                    PBRPushConstants constants;
                    constants.ModelViewProjection = ToMatrix4(viewProjection * object.WorldMatrix);
                    constants.BaseColorFactor = gpuMaterial->BaseColorFactor;
                    constants.MaterialFactors = Vec4 {
                        gpuMaterial->MetallicFactor,
                        gpuMaterial->RoughnessFactor,
                        gpuMaterial->AlphaCutoff,
                        0.0f
                    };
                    commandList->PushConstants(RHIShaderStageFlags::AllGraphics, &constants, sizeof(constants));

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
