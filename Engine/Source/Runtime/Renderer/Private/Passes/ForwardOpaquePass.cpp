#include "ForwardOpaquePass.h"

#include "../Resources/RenderResourceContext.h"
#include "../Resources/GraphicsPipelineStateKey.h"
#include "../Resources/RenderFrameResources.h"
#include "../Resources/RenderPipelineStateCache.h"
#include "../Resources/RenderMaterialSystem.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderShaderTypes.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"
#include "../Pipeline/RenderFrameContext.h"

#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
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
                if (commandList == nullptr || frameContext.Device == nullptr || !resources.IsValid())
                {
                    return;
                }

                // ShadowDepthPass sets a depth-only render output. Restore the full
                // color+depth binding so the ForwardOpaque pipeline (colorAttachmentCount=1)
                // sees a matching VkRenderingInfo.
                commandList->SetRenderOutput(frameContext.Output);

                RHIBindGroup* frameBindGroup =
                    resources.FrameResources->GetFrameBindGroup(frameContext.FrameIndex);
                if (frameBindGroup == nullptr)
                {
                    return;
                }

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

                    GraphicsPipelineStateKey pipelineKey;
                    pipelineKey.PassKind = RenderPassKind::ForwardOpaque;
                    pipelineKey.ShadingModel = materialDesc->ShadingModel;
                    pipelineKey.AlphaMode = materialDesc->AlphaMode;
                    pipelineKey.VertexLayout = VertexLayoutKind::MeshVertex;
                    pipelineKey.ColorFormat = frameContext.Output.ColorFormat;
                    pipelineKey.DepthFormat = RHIFormat::D32Float;
                    pipelineKey.DepthTestEnabled = true;
                    pipelineKey.DepthWriteEnabled = materialDesc->AlphaMode != MaterialAlphaMode::Blend;
                    pipelineKey.BlendEnabled = materialDesc->AlphaMode == MaterialAlphaMode::Blend;
                    pipelineKey.DoubleSided = materialDesc->DoubleSided;

                    RHIPipeline* pipeline =
                        resources.PipelineStates->GetOrCreateGraphicsPipeline(pipelineKey);
                    if (pipeline == nullptr)
                    {
                        continue;
                    }

                    commandList->SetGraphicsPipeline(pipeline);
                    commandList->SetVertexBuffer(mesh->VertexBuffer.get());
                    commandList->SetIndexBuffer(mesh->IndexBuffer.get(), mesh->IndexFormat);

                    // Set 0: per-frame data shared by all objects in this pass.
                    // Includes camera and scene lighting.
                    commandList->SetBindGroup(0, frameBindGroup);
                    commandList->SetBindGroup(1, bindGroup);

                    PBRPushConstants constants;
                    constants.Model = object.WorldMatrix;
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
