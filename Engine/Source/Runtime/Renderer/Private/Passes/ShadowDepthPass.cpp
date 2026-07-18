#include "ShadowDepthPass.h"

#include "../Resources/RenderPipelineStateCache.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderShaderTypes.h"
#include "../Resources/RenderResourceContext.h"
#include "../Resources/RenderShaderTypes.h"
#include "../Shadows/RenderShadowManager.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    void AddShadowDepthPass(
        RenderGraph& graph,
        const RenderFrameContext& frame,
        const RenderScene& scene,
        RenderResourceContext& resources)
    {
        if (!resources.IsValid())
        {
            return;
        }
        const RenderShadowManager* shadowManager = resources.ShadowManager;
        
        if (!shadowManager->HasDirectionalShadow())
        {
            return;
        }

        const RenderDirectionalShadowFrameData& dir = shadowManager->GetFrameData().Directional;

        for (u32 cascadeIndex = 0; cascadeIndex < dir.CascadeCount; ++cascadeIndex)
        {
            const RenderShadowCascade& cascade = dir.Cascades[cascadeIndex];
            RHITextureView* depthView = dir.CascadeDepthViews[cascadeIndex];
            if (depthView == nullptr)
            {
                continue;
            }

            RHIPipeline* depthPipeline =
                resources.PipelineStates->GetOrCreateShadowDepthPipeline(
                    /* colorFormat */ RHIFormat::Undefined,
                    /* depthFormat */ RHIFormat::D32Float);
            if (depthPipeline == nullptr)
            {
                continue;
            }

            std::string passName = "ShadowDepthPass.C" + std::to_string(cascadeIndex);
            RenderGraphPassDesc desc;
            desc.Name = passName.c_str();
            desc.Type = RenderGraphPassType::Graphics;
            // Optional: declare depth-attachment access for the future RenderGraph resource tracker.
            // desc.DepthAttachment = depthView;

            graph.AddPass(
                desc,
                [depthView](RenderGraphBuilder&)
                {
                    // TODO Stage 10+: declare explicit depth attachment access here.
                    (void)depthView;
                },
                [depthView, depthPipeline, &cascade, &scene, &resources, cascadeIndex](RenderGraphContext& context)
                {
                    RHICommandList* commandList = context.GetCommandList();
                    if (commandList == nullptr || depthPipeline == nullptr)
                    {
                        return;
                    }

                    // Set the depth-only render output to the cascade's per-layer view.
                    RHIRenderOutputDesc output;
                    output.ColorTargetView = nullptr;     // depth-only
                    output.DepthTargetView = depthView;
                    output.Viewport        = RHIRect2D { 0, 0, cascade.Resolution, cascade.Resolution };
                    output.ColorFormat     = RHIFormat::Undefined;
                    output.DepthFormat     = RHIFormat::D32Float;
                    output.RenderToSwapchain = false;
                    commandList->SetRenderOutput(output);

                    commandList->SetGraphicsPipeline(depthPipeline);

                    // Iterate shadow-casting objects.
                    for (const RenderObject& object : scene.OpaqueObjects)
                    {
                        if (!object.Visible) continue;
                        if (!object.CastShadow) continue;
                        if (!object.Mesh.IsValid()) continue;

                        const RenderMesh* mesh = resources.Meshes->GetMesh(object.Mesh);
                        if (mesh == nullptr || !mesh->VertexBuffer || !mesh->IndexBuffer)
                        {
                            continue;
                        }

                        // Per-object shader constants.
                        ShadowDepthPushConstants constants;
                        constants.Model               = object.WorldMatrix;
                        constants.LightViewProjection = cascade.LightViewProjection;
                        constants.CascadeIndex        = cascadeIndex;
                        commandList->PushConstants(
                            RHIShaderStageFlags::Vertex,
                            &constants,
                            sizeof(constants));

                        commandList->SetVertexBuffer(mesh->VertexBuffer.get());
                        commandList->SetIndexBuffer(mesh->IndexBuffer.get(), mesh->IndexFormat);

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
                }
            );
        }

    }
}