# CSM_06 — ShadowDepthPass and Pipeline

## Goal

The `ShadowDepthPass` writes per-cascade depth to the shadow texture array. It does not compute cascade splits, does not allocate resources, and does not know the light direction. It receives the precomputed `RenderShadowFrameData` from `RenderShadowManager` and renders each cascade's depth-only output.

The pass executes before the `ForwardOpaquePass` in `ForwardRenderPipeline`.

## Files to Add / Fill

- `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.h` (header is `#pragma once` only; add the API).
- `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp` (does not exist; create it).
- `Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp` (add the call to `AddShadowDepthPass`).
- `Engine/Shaders/Passes/DepthOnly.slang` (empty; create the depth-only VS/FS).

## File to Modify (for shader compilation)

The shader source must be added to the project's shader build pipeline. The existing shaders are listed in the project's shader discovery path. After creating `DepthOnly.slang`, ensure it is included in the build's `RenderShaderLibrary` registration or in the explicit shader list.

## API

```cpp
// ShadowDepthPass.h
#pragma once

namespace XEngine
{
    class RenderGraph;
    struct RenderFrameContext;
    struct RenderScene;
    struct RenderResourceContext;

    void AddShadowDepthPass(
        RenderGraph& graph,
        const RenderFrameContext& frame,
        const RenderScene& scene,
        RenderResourceContext& resources);
}
```

This mirrors the existing `AddForwardOpaquePass` signature pattern in `Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.h`.

## `ShadowDepthPass.cpp` Implementation

Pseudocode (matching the existing `AddForwardOpaquePass` style):

```cpp
#include "ShadowDepthPass.h"

#include "../Resources/RenderPipelineStateCache.h"
#include "../Resources/RenderMeshManager.h"
#include "../Resources/RenderShaderTypes.h"
#include "../Resources/RenderResourceContext.h"
#include "../Shadows/RenderShadowManager.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RenderGraphContext.h"

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
        if (!resources.PipelineStates)
        {
            return;
        }
        const RenderShadowManager* shadowManager = resources.ShadowManager;
        if (shadowManager == nullptr || !shadowManager->HasDirectionalShadow())
        {
            return;
        }

        const RenderDirectionalShadowFrameData& dir =
            shadowManager->GetFrameData().Directional;

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

            const u32 passIndex = graph.GetNextPassIndex();
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
                [depthPipeline, &cascade, &scene, &resources, cascadeIndex](RenderGraphContext& context)
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
                        if (object.Mesh.IsValid() == false) continue;

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
                });
        }
    }
}
```

## `RenderPipelineStateCache` Extension

The cache must add a method to provide a depth-only graphics pipeline for the shadow pass. The method does not exist yet; add it to:

- `Engine/Source/Runtime/Renderer/Private/Resources/RenderPipelineStateCache.h`
- `Engine/Source/Runtime/Renderer/Private/Resources/RenderPipelineStateCache.cpp`

```cpp
// In the header:
RHIPipeline* GetOrCreateShadowDepthPipeline(RHIFormat colorFormat, RHIFormat depthFormat);
```

The implementation:

```cpp
RHIPipeline* RenderPipelineStateCache::GetOrCreateShadowDepthPipeline(
    RHIFormat colorFormat, RHIFormat depthFormat)
{
    GraphicsPipelineStateKey key;
    key.PassKind         = RenderPassKind::ShadowDepth;
    key.ShadingModel     = MaterialShadingModel::Unlit;     // depth-only is unlit
    key.AlphaMode        = MaterialAlphaMode::Opaque;
    key.VertexLayout     = VertexLayoutKind::MeshVertex;
    key.ColorFormat      = colorFormat;     // typically Undefined
    key.DepthFormat      = depthFormat;     // typically D32Float
    key.DepthTestEnabled = true;
    key.DepthWriteEnabled = true;
    key.BlendEnabled     = false;
    key.DoubleSided      = false;
    key.HasColorAttachment = false;        // depth-only
    return GetOrCreateGraphicsPipeline(key);
}
```

This requires:

1. `RenderPassKind::ShadowDepth` to be added to the `RenderPassKind` enum (in the same header as `ForwardOpaque`).
2. The `GraphicsPipelineStateKey` to include a `HasColorAttachment` field. The current key may not have this — add it if needed.
3. The pipeline cache's `GetOrCreateGraphicsPipeline` to handle the depth-only case. Verify that `RHIGraphicsPipelineDesc::HasColorAttachment = false` works (it already does — see `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h` line 360 and the validation in `Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp` line 261).
4. The pipeline cache's `GetOrCreateGraphicsPipeline` to bind the `DepthOnly` shader. This means the cache must know about the `DepthOnly` shader, not just material shaders. The cleanest way is to treat `RenderPassKind::ShadowDepth` as a special case and call the factory with a custom `RHIGraphicsPipelineDesc` that references the depth-only shaders.

Alternative: skip the pipeline cache for the shadow pass and call `factory.CreateGraphicsPipeline` directly inside `AddShadowDepthPass`. This is simpler and acceptable for V0. The shadow pass does not need the full pipeline cache infrastructure; there is exactly one shadow depth pipeline (the depth format is fixed to D32Float and the only "vertex layout" is the standard mesh layout). Recommended for V0:

```cpp
// Inside AddShadowDepthPass, lazily create the pipeline once and cache it
// in a static or in the pass.
static std::shared_ptr<RHIPipeline> s_ShadowDepthPipeline;
static RHIFormat s_ShadowDepthPipelineFormat = RHIFormat::Undefined;

if (s_ShadowDepthPipeline == nullptr || s_ShadowDepthPipelineFormat != RHIFormat::D32Float)
{
    // Acquire shaders.
    RHIShader* vertexShader = resources.Shaders->GetShader(ShaderKey::DepthOnlyVertex);
    RHIShader* fragmentShader = resources.Shaders->GetShader(ShaderKey::DepthOnlyFragment);
    if (vertexShader == nullptr || fragmentShader == nullptr)
    {
        return;
    }

    RHIGraphicsPipelineDesc desc;
    desc.VertexShader    = vertexShader;
    desc.FragmentShader  = fragmentShader;
    desc.ColorFormat     = RHIFormat::Undefined;
    desc.DepthFormat     = RHIFormat::D32Float;
    desc.HasColorAttachment = false;
    desc.EnableDepthTest   = true;
    desc.EnableDepthWrite  = true;
    desc.VertexLayout.Stride = sizeof(MeshVertex);  // standard mesh layout
    desc.PushConstantSize    = sizeof(ShadowDepthPushConstants);
    desc.PushConstantStages  = RHIShaderStageFlags::Vertex;
    desc.DebugName = "ShadowDepthPipeline";

    s_ShadowDepthPipeline = frame.Device->GetResourceFactory().CreateGraphicsPipeline(desc);
    s_ShadowDepthPipelineFormat = RHIFormat::D32Float;
}

RHIPipeline* depthPipeline = s_ShadowDepthPipeline.get();
```

The shader keys `ShaderKey::DepthOnlyVertex` and `ShaderKey::DepthOnlyFragment` must be added to the `RenderShaderLibrary` if they do not already exist. The shaders are loaded from `Engine/Shaders/Passes/DepthOnly.slang`.

This is the recommended path: it bypasses the pipeline cache for the shadow pass in V0, where there is exactly one depth-only pipeline.

## `ForwardRenderPipeline` Integration

In `Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp`:

```cpp
#include "../Passes/ShadowDepthPass.h"

// In Render():
if (resources.ShadowManager != nullptr && resources.ShadowManager->HasDirectionalShadow())
{
    AddShadowDepthPass(m_Graph, frame, scene, resources);
}
AddForwardOpaquePass(m_Graph, frame, scene, resources);
if (frame.Output.RenderToSwapchain)
{
    AddPresentPass(m_Graph);
}
```

The shadow pass executes first because Vulkan's dynamic rendering does not have implicit dependencies; each pass is responsible for the layout transition of its attachments. The depth-only pass writes to `SHADER_READ_ONLY_OPTIMAL` (sampled by the next pass) via the implicit transition in `VulkanBindGroup` for the whole-array sampled view.

If a real RenderGraph V1 is later added, the depth attachment would be declared as `Access::Write` and the frame bind group's shadow sampled view as `Access::Read`, with a barrier in between.

## `RenderResourceContext` Field

Add to `Engine/Source/Runtime/Renderer/Private/Resources/RenderResourceContext.h`:

```cpp
class RenderShadowManager;
struct RenderResourceContext
{
    // ... existing fields ...
    RenderShadowManager* ShadowManager = nullptr; // NEW
};
```

`RenderSystem` initializes it:

```cpp
// In RenderSystem::Impl::Render or in RenderSystem::OnCreate:
impl.Resources.ShadowManager = impl.ShadowManager.get();
```

`RenderSystem::Impl` must own the manager:

```cpp
std::unique_ptr<RenderShadowManager> ShadowManager;
```

Initialized in `OnCreate` and shut down in `Shutdown`.

## `ShadowDepthPushConstants`

Define in `Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderTypes.h` (or a new file `ShadowShaderTypes.h`):

```cpp
struct ShadowDepthPushConstants
{
    Mat4 Model;                  // object world matrix
    Mat4 LightViewProjection;    // cascade's light view-projection
    u32  CascadeIndex;           // not used by depth-only VS, kept for future
    u32  _pad0;
    u32  _pad1;
    u32  _pad2;
};
static_assert(sizeof(ShadowDepthPushConstants) % 16 == 0,
              "Push constants must be 16-byte aligned");
```

Total size: 64 + 64 + 16 = 144 bytes. Within Vulkan's `maxPushConstantsSize` (default 128 bytes minimum, often 256 on desktop).

## Shader: `DepthOnly.slang`

```hlsl
#include "../Common/Types.slang"

struct ShadowDepthPushConstants
{
    float4x4 Model;
    float4x4 LightViewProjection;
    uint CascadeIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};
ConstantBuffer<ShadowDepthPushConstants> g_Push;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(g_Push.Model, float4(input.position, 1.0));
    output.position = mul(g_Push.LightViewProjection, worldPos);
    return output;
}

[shader("fragment")]
void fragmentMain(VSOutput input)
{
    // Depth-only: just write SV_Depth (no color write).
    // Slang/SPIRV backend handles SV_Depth emission from position.
}
```

Note: Slang may require a return value for the fragment shader. If so, return `float4(0, 0, 0, 0)` — but the depth-only pipeline has `HasColorAttachment = false`, so the color attachment is never written. The return value is unused. Some Slang versions require an empty fragment; in that case, replace with `void fragmentMain` if supported, or add a dummy `return float4(0,0,0,0);` and rely on dead-code elimination.

For safety, use:

```hlsl
[shader("fragment")]
[ForceInline]
void fragmentMain(VSOutput input)
{
    // no-op; SV_Depth is generated from VSOutput.position.
}
```

The `[ForceInline]` attribute is a Slang-specific optimization hint.

## `VSInput` and `VSOutput` Compatibility

The depth-only shader uses the same `VSInput` from `Engine/Shaders/Common/Types.slang` (POSITION, NORMAL, TEXCOORD0). It does not need UVs or normals, but matching the input keeps the pipeline layouts compatible with the mesh vertex layout. The unused varyings are optimized out by the shader compiler.

## Common Mistakes

1. **Forgetting `output.position = mul(g_Push.LightViewProjection, worldPos)`.** The vertex shader must output the position in light space, not world or camera space. A common mistake is to write `output.position = mul(g_FrameData.Camera.ViewProjection, worldPos)` — that renders the scene from the camera into the shadow map, which produces black shadows.

2. **Forgetting to set `RHIRenderOutputDesc::RenderToSwapchain = false`.** If the shadow pass is treated as a swapchain pass, the depth attachment is ignored. Setting it to `false` makes the command list use the per-layer depth view.

3. **Setting `output.ColorFormat = RHIFormat::Undefined` and `output.DepthFormat = RHIFormat::Undefined`.** The depth format must be the actual depth format (`D32Float`).

4. **Forgetting to call `commandList->SetRenderOutput` per cascade.** Each cascade has a different `RHITextureView` and a different viewport. The output must be set per cascade.

5. **Not bounding the viewport to the cascade resolution.** With `Viewport = { 0, 0, swapchainWidth, swapchainHeight }` (the frame's default), the shadow pass would render outside the shadow map. Set the viewport to the cascade's resolution.

6. **Using `RHIPipeline* GetOrCreateGraphicsPipeline` from the pipeline cache for the shadow pass.** The cache is designed around material-driven keys and may not handle the depth-only case cleanly. Use the explicit factory call documented above for V0.

7. **Not handling `cascadeIndex` overflow.** The planner guarantees `cascadeIndex < dir.CascadeCount`, but the pass should defend against it.

8. **Drawing objects with `Visible == false` or `CastShadow == false`.** Always filter on these.

9. **Not clearing the depth attachment before rendering.** The depth attachment is in `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL` from the previous pass's transition. Clear it to `1.0` (the default for reverse-Z far) at the start of each cascade. The `RHICommandList` does not have a `ClearDepthAttachment` method in V0; the per-pass `loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR` is set by the `BeginRenderingIfNeeded` path in `VulkanCommandList::BeginRenderingIfNeeded` (see line 360 of `VulkanCommandList.cpp`). Verify that this is the case for non-swapchain outputs as well. If it is not, the shadow map will start with garbage depth.
